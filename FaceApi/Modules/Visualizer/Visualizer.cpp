#define _USE_MATH_DEFINES

#include "Modules/Visualizer/Visualizer.h"

#include "Common/Configuration.h"
#include "Common/PoseUtil.h"
#include "Common/ShapeUtil.h"
#include "Framework/Profiler.h"

#include <iomanip>

namespace face
{
  fw::ErrorCode Visualizer::InitializeInternal(const cv::FileNode& iSettings)
  {
    mColorsOfAxes.push_back({ 255, 255, 255 });
    mColorsOfAxes.push_back({ 255, 0, 0 });
    mColorsOfAxes.push_back({ 0, 255, 0 });
    mColorsOfAxes.push_back({ 0, 0, 255 });

    return fw::ErrorCode::OK;
  }

  ImageMessage::Shared Visualizer::Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iUsers)
  {
    if (!iImage || iImage->IsEmpty()) return nullptr;

    cv::Mat resultImage = iImage->GetFrameBGR()/*.clone()*/;

    if (iUsers && !iUsers->IsEmpty())
    {
      FACE_PROFILER(4_Draw);

      const auto& activeUsers = iUsers->GetActiveUsers();
      for (auto& user : activeUsers)
      {
        if (!user) continue;

        DrawShapeModel(*user, resultImage);

        DrawBoundingBox(*user, resultImage, 5, 1);

        DrawUserData(*user, resultImage);

        if (Configuration::GetInstance().GetOutput().verbose)
          DrawAxes(*user, resultImage);
      }
    }

    const long long runtimeMs = std::llabs(fw::get_current_time() - iImage->GetTimestamp());

    DrawGeneral(runtimeMs, iImage->GetFrameId(), resultImage);

    return std::make_shared<ImageMessage>(resultImage, iImage->GetFrameId(), iImage->GetTimestamp());
  }

  void Visualizer::DrawShapeModel(const User& iUser, cv::Mat& oImage) const
  {
    const auto& shape2D = iUser.GetShape2D();
    const auto& shape3D = iUser.GetShape3D();
    const auto& connections = ShapeUtil::GetInstance().GetConnections();

    cv::Mat colorMap;
    CreateShapeColorMap(shape3D, colorMap);

    // draw connections
    for (const auto& c : connections)
    {
      cv::Vec3b color((colorMap.at<cv::Vec3b>(c.first, 0) + colorMap.at<cv::Vec3b>(c.second, 0)) / 2);
      cv::line(oImage, shape2D[c.first], shape2D[c.second], cv::Scalar(color), 2, cv::LINE_AA);
    }

    // draw points
    for (int i = 0; i < shape2D.size(); i++)
    {
      cv::drawMarker(oImage, shape2D[i], colorMap.at<cv::Vec3b>(i, 0), cv::MarkerTypes::MARKER_CROSS, 5, 1, cv::LINE_AA);
    }
  }

  void Visualizer::CreateShapeColorMap(const fw::ocv::VectorPt3D& iShape3D, cv::Mat& oColorMap) const
  {
    const std::size_t n = iShape3D.size();

    float minZ = (std::numeric_limits<float>::max)();
    float maxZ = (std::numeric_limits<float>::min)();

    for (const auto& pt : iShape3D)
    {
      const float z = static_cast<float>(pt.z);
      if (z < minZ) minZ = z;
      if (z > maxZ) maxZ = z;
    }

    cv::Mat grayMap(n, 1, CV_8UC1);
    for (int i = 0; i < n; i++)
    {
      const float z = static_cast<float>(iShape3D[i].z);
      const float val = fw::scale_interval(z, minZ, maxZ, 0.0F, 255.0F);
      grayMap.at<uchar>(i, 0) = static_cast<uchar>(255.0F - val);
    }

    cv::applyColorMap(grayMap, oColorMap, cv::COLORMAP_COOL);

    for (int i = 0; i < n; i++)
    {
      const cv::Vec3b& c = oColorMap.at<cv::Vec3b>(i, 0);
      oColorMap.at<cv::Vec3b>(i, 0) = cv::Vec3b(255 - c[0], 255 - c[1], 255 - c[2]);
    }
  }

  void Visualizer::DrawUserData(const User& iUser, cv::Mat& oImage) const
  {
    const auto& faceRect = iUser.GetFaceRect();
    cv::Point textPt(faceRect.x + faceRect.width, faceRect.y + 5);
    std::stringstream ss;

    ss << "UserID: " << iUser.GetUserId();
    fw::ocv::put_text(ss.str(), textPt, oImage);
    ss.str("");

    textPt.y += 15;
    ss << "Detected: " << cvRound((iUser.GetLastUpdateTs() - iUser.GetLastDetectionTs()) / 1000.0) << " sec";
    fw::ocv::put_text(ss.str(), textPt, oImage);
    ss.str("");

    textPt.y += 15;
    ss << "Alive: " << cvRound((iUser.GetLastUpdateTs() - iUser.GetCreationTs()) / 1000.0) << " sec";
    fw::ocv::put_text(ss.str(), textPt, oImage);
    ss.str("");
  }

  void Visualizer::DrawBoundingBox(const User& iUser, cv::Mat& oImage, int iSegmentWidth/* = 5*/, int iThickness/* = 1*/) const
  {
    const cv::Scalar color(240, 255, 150);
    const auto& connections = PoseUtil::GetInstance().GetConnections();

    fw::ocv::VectorPt2D faceBoxProj;
    fw::ocv::project_point(iUser.GetFaceBox(), iUser.GetRvec(), iUser.GetTvec(), iUser.GetCameraMatrix(), faceBoxProj);

    for (const auto& c : connections)
    {
      fw::ocv::draw_dotted_line(oImage, faceBoxProj[c.first], faceBoxProj[c.second], color, iSegmentWidth, iThickness);
    }

    for (const auto& pt : faceBoxProj)
    {
      cv::circle(oImage, pt, 3, color, -1);
    }
  }

  void Visualizer::DrawAxes(const User& iUser, cv::Mat& oImage) const
  {
    static const fw::ocv::VectorPt3D sAxes3D = PoseUtil::GetInstance().GetAxes3D();

    fw::ocv::VectorPt2D axes2D;
    fw::ocv::project_point(sAxes3D, iUser.GetRvec(), iUser.GetTvec(), iUser.GetCameraMatrix(), axes2D);

    const cv::Point2d minPt = iUser.GetFaceRect().tl() - cv::Point(10, 10);
    cv::Point2d shiftPt(axes2D[0] - minPt);

    for (size_t i = 0; i < axes2D.size(); i++)
    {
      cv::arrowedLine(oImage, axes2D[0] - shiftPt, axes2D[i] - shiftPt, cv::Scalar::all(50), 3, cv::LINE_AA, 0, 0.075);
      cv::arrowedLine(oImage, axes2D[0] - shiftPt, axes2D[i] - shiftPt, mColorsOfAxes[i], 2, cv::LINE_AA, 0, 0.075);
    }

    shiftPt += cv::Point2d(10.0, 10.0);

    const cv::Vec3d& RPY = iUser.GetRPY();
    const cv::Vec3d& position3D = iUser.GetPosition3D();

    fw::ocv::put_text("O", axes2D[0] - shiftPt, oImage);

    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed << "Pitch (" << FW_RAD_TO_DEG(RPY[1]) << ", " << position3D[0] << ")";
    fw::ocv::put_text(ss.str(), axes2D[1] - shiftPt, oImage);
    ss.str("");

    ss << std::setprecision(2) << std::fixed << "Yaw (" << FW_RAD_TO_DEG(RPY[2]) << ", " << position3D[1] << ")";
    fw::ocv::put_text(ss.str(), axes2D[2] - shiftPt, oImage);
    ss.str("");

    ss << std::setprecision(2) << std::fixed << "Roll (" << FW_RAD_TO_DEG(RPY[0]) << ", " << position3D[2] << ")";
    fw::ocv::put_text(ss.str(), axes2D[3] - shiftPt, oImage);
    ss.str("");
  }

  void Visualizer::DrawGeneral(double iRuntime, int iFrameID, cv::Mat& oImage) const
  {
    static double sMinRuntime = iRuntime;
    static double sMaxRuntime = iRuntime;

    if (iRuntime < sMinRuntime) sMinRuntime = iRuntime;
    if (iRuntime > sMaxRuntime) sMaxRuntime = iRuntime;

    const int h = 5;

    cv::Scalar c = fw::ocv::get_color(iRuntime, sMinRuntime, sMaxRuntime);
    cv::Rect r(1, oImage.rows - h, oImage.cols - 1, h);

    cv::rectangle(oImage, r, c, 2);
    cv::rectangle(oImage, r, c, -1);

    std::stringstream ss;
    ss << "Frame number: " << iFrameID << " (" << cvRound(iRuntime) << " ms @ " << cvRound(1000.0 / iRuntime) << " FPS) - Queue size: " << mQueueSize;
    fw::ocv::put_text(ss.str(), { 10, oImage.rows - 15 }, oImage);

#ifdef ENABLE_FACE_PROFILER
    if (Configuration::GetInstance().GetOutput().verbose)
    {
      const int barHeight = 15;
      const auto& lastMeasurement = fw::ProfilerDatabase::GetInstance().GetLastMeasurement();

      int idx = 0;
      for (auto& m : lastMeasurement)
      {
        std::stringstream ss;
        ss << "- " << m.first << "(" << (iFrameID - m.second.first) << "): " << cvRound(m.second.second) << " ms";
        fw::ocv::put_text(ss.str(), { 10, (barHeight * ++idx) + 20 }, oImage);
      }
    }
#endif
  }
}
