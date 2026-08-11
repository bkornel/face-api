#include "Framework/ErrorCode.h"
#include "Framework/UtilMath.h"
#include "Framework/UtilTime.h"
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
    mColorsOfAxes.emplace_back(255, 255, 255);
    mColorsOfAxes.emplace_back(255, 0, 0);
    mColorsOfAxes.emplace_back(0, 255, 0);
    mColorsOfAxes.emplace_back(0, 0, 255);

    return fw::ErrorCode::OK;
  }

  std::shared_ptr<ImageMessage> Visualizer::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if (!iImage || iImage->IsEmpty()) return nullptr;

    // Deep copy: assignment shares the buffer with the frame other modules read.
    cv::Mat resultImage = iImage->GetFrameBGR().clone();

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

        if (mVerboseMode)
          DrawAxes(*user, resultImage);
      }
    }

    DrawGeneral(iImage, resultImage);

    // Moved in: the buffer was drawn here and is not touched afterwards, so there is no
    // reason for the message to clone it again
    return std::make_shared<ImageMessage>(std::move(resultImage), iImage->GetFrameId(), iImage->GetTimestamp());
  }

  void Visualizer::DrawShapeModel(const User& iUser, cv::Mat& oImage) const
  {
    const auto& shape2D = iUser.GetShape2D();
    const auto& shape3D = iUser.GetShape3D();
    const auto& connections = ShapeUtil::GetInstance().GetConnections();

    // A user that has just been detected, or whose fit failed, has no shape yet
    if (shape2D.empty() || shape3D.size() != shape2D.size()) return;

    cv::Mat colorMap;
    CreateShapeColorMap(shape3D, colorMap);

    const int pointCount = static_cast<int>(shape2D.size());

    // draw connections
    for (const auto& c : connections)
    {
      // The connection list comes from the model file, it may not match this shape
      if (c.first < 0 || c.first >= pointCount || c.second < 0 || c.second >= pointCount)
        continue;

      cv::Vec3b color((colorMap.at<cv::Vec3b>(c.first, 0) + colorMap.at<cv::Vec3b>(c.second, 0)) / 2);
      cv::line(oImage, shape2D[c.first], shape2D[c.second], cv::Scalar(color), 2, cv::LINE_AA);
    }

    // draw points
    for (int i = 0; i < pointCount; i++)
    {
      cv::drawMarker(oImage, shape2D[i], colorMap.at<cv::Vec3b>(i, 0), cv::MarkerTypes::MARKER_CROSS, 5, 1, cv::LINE_AA);
    }
  }

  void Visualizer::CreateShapeColorMap(const fw::ocv::VectorPt3D& iShape3D, cv::Mat& oColorMap) const
  {
    const std::size_t n = iShape3D.size();

    float minZ = (std::numeric_limits<float>::max)();
    // lowest(), not min(): min() is the smallest positive value, not the most negative.
    float maxZ = (std::numeric_limits<float>::lowest)();

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

  void Visualizer::DrawBoundingBox(const User& iUser, cv::Mat& oImage, int iSegmentWidth /* = 5*/, int iThickness /* = 1*/) const
  {
    const cv::Scalar color(240, 255, 150);
    const auto& connections = PoseUtil::GetInstance().GetConnections();

    // No pose estimated for this user yet, so there is no box to project
    if (iUser.GetFaceBox().empty() || iUser.GetRvec().empty() || iUser.GetTvec().empty() || iUser.GetCameraMatrix().empty())
      return;

    fw::ocv::VectorPt2D faceBoxProj;
    fw::ocv::project_point(iUser.GetFaceBox(), iUser.GetRvec(), iUser.GetTvec(), iUser.GetCameraMatrix(), faceBoxProj);

    const int cornerCount = static_cast<int>(faceBoxProj.size());

    for (const auto& c : connections)
    {
      if (c.first < 0 || c.first >= cornerCount || c.second < 0 || c.second >= cornerCount)
        continue;

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

    if (iUser.GetRvec().empty() || iUser.GetTvec().empty() || iUser.GetCameraMatrix().empty())
      return;

    fw::ocv::VectorPt2D axes2D;
    fw::ocv::project_point(sAxes3D, iUser.GetRvec(), iUser.GetTvec(), iUser.GetCameraMatrix(), axes2D);

    if (axes2D.size() < sAxes3D.size() || axes2D.size() > mColorsOfAxes.size()) return;

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
    ss << std::setprecision(2) << std::fixed << "Pitch (" << fw::rad_to_deg(RPY[1]) << ", " << position3D[0] << ")";
    fw::ocv::put_text(ss.str(), axes2D[1] - shiftPt, oImage);
    ss.str("");

    ss << std::setprecision(2) << std::fixed << "Yaw (" << fw::rad_to_deg(RPY[2]) << ", " << position3D[1] << ")";
    fw::ocv::put_text(ss.str(), axes2D[2] - shiftPt, oImage);
    ss.str("");

    ss << std::setprecision(2) << std::fixed << "Roll (" << fw::rad_to_deg(RPY[0]) << ", " << position3D[2] << ")";
    fw::ocv::put_text(ss.str(), axes2D[3] - shiftPt, oImage);
    ss.str("");
  }

  void Visualizer::DrawGeneral(std::shared_ptr<ImageMessage> iImage, cv::Mat& oImage)
  {
    const double runtimeMs = std::llabs(fw::get_current_time() - iImage->GetTimestamp());

    // Members, not function statics: resettable with the rest of the module.
    if (runtimeMs < mMinRuntimeMs) mMinRuntimeMs = runtimeMs;
    if (runtimeMs > mMaxRuntimeMs) mMaxRuntimeMs = runtimeMs;

    const int h = 5;

    cv::Scalar c = fw::ocv::get_color(runtimeMs, mMinRuntimeMs, mMaxRuntimeMs);
    cv::Rect r(1, oImage.rows - h, oImage.cols - 1, h);

    cv::rectangle(oImage, r, c, 2);
    cv::rectangle(oImage, r, c, -1);

    std::stringstream ss;
    ss << "Frame number: " << iImage->GetFrameId() << " (" << cvRound(runtimeMs) << " ms @ ";

    // A frame can be processed inside the same millisecond the timestamp was taken.
    if (runtimeMs > 0.0)
      ss << cvRound(1000.0 / runtimeMs);
    else
      ss << "-";

    ss << " FPS) - Queue size: " << iImage->GetQueueData().size;

    fw::ocv::put_text(ss.str(), { 10, oImage.rows - 15 }, oImage);

#ifdef ENABLE_FACE_PROFILER
    if (mVerboseMode)
    {
      const int barHeight = 15;
      const auto& lastMeasurement = fw::ProfilerDatabase::GetInstance().GetLastMeasurement();

      int idx = 0;
      for (auto& m : lastMeasurement)
      {
        std::stringstream ss;
        ss << "- " << m.first << "(" << (iImage->GetFrameId() - m.second.first) << "): " << cvRound(m.second.second) << " ms";

        fw::ocv::put_text(ss.str(), { 10, (barHeight * ++idx) + 20 }, oImage);
      }
    }
#endif
  }
}