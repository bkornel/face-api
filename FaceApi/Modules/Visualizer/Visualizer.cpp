#include "Framework/Imaging/Drawing.h"
#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Projection.h"
#include "Framework/ErrorCode.h"
#include "Framework/MathExtensions.h"
#include "Framework/Settings.h"
#include "Framework/TimeExtensions.h"
#include "Modules/Visualizer/Visualizer.h"

#include "Configuration.h"
#include "Model/PoseGeometry.h"
#include "Model/FaceModel.h"
#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace face
{
  // The 68-point layout as the parts of a face. Every group is one stroke, which is what
  // makes the drawing read as a face rather than as a cloud of points.
  const std::vector<Visualizer::FeatureGroup> Visualizer::sFeatureGroups = {
    { 0, 16, false, false, 2 },  // Jaw
    { 17, 21, false, false, 2 }, // Right eyebrow
    { 22, 26, false, false, 2 }, // Left eyebrow
    { 27, 30, false, false, 2 }, // Nose bridge
    { 31, 35, false, false, 2 }, // Nostrils
    { 36, 41, true, true, 1 },   // Right eye
    { 42, 47, true, true, 1 },   // Left eye
    { 48, 59, true, false, 2 },  // Outer lip
    { 60, 67, true, true, 1 }    // Inner lip
  };

  const std::size_t Visualizer::sRuntimeHistorySize = 120U;

  fw::ErrorCode Visualizer::InitializeInternal(const cv::FileNode& iSettings)
  {
    // Assigned, not appended: a second Initialize() used to grow the list and shift every
    // axis onto the wrong colour.
    mColorsOfAxes = {
      { 255, 255, 255 },
      { 90, 90, 245 },  // X, red
      { 90, 225, 90 },  // Y, green
      { 245, 175, 60 }  // Z, blue
    };

    // Blue-green-red order, and picked to stay apart on skin tones and to survive the
    // glow without washing out to white
    mAccentPalette = {
      { 70, 200, 255 },  // amber
      { 150, 240, 120 }, // mint
      { 240, 140, 200 }, // orchid
      { 255, 190, 90 },  // sky
      { 120, 120, 250 }  // coral
    };

    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "glow", value))
        mDrawGlow = fw::str::convert_to_boolean(value);

      if (fw::get_value(iSettings, "panel", value))
        mDrawPanel = fw::str::convert_to_boolean(value);

      if (fw::get_value(iSettings, "poseBox", value))
        mDrawPoseBox = fw::str::convert_to_boolean(value);
    }

    return fw::ErrorCode::OK;
  }

  cv::Scalar Visualizer::GetAccentColor(int iUserId) const
  {
    if (mAccentPalette.empty()) return { 255, 255, 255 };

    const std::size_t index = static_cast<std::size_t>(iUserId < 0 ? -iUserId : iUserId) % mAccentPalette.size();
    return mAccentPalette[index];
  }

  std::vector<double> Visualizer::GetDepthWeights(const fw::VectorPt3D& iShape3D) const
  {
    std::vector<double> weights(iShape3D.size(), 1.0);
    if (iShape3D.empty()) return weights;

    double minZ = iShape3D[0].z;
    double maxZ = iShape3D[0].z;

    for (const auto& pt : iShape3D)
    {
      minZ = (std::min)(minZ, pt.z);
      maxZ = (std::max)(maxZ, pt.z);
    }

    const double range = maxZ - minZ;

    // A flat shape carries no depth information, so it is drawn evenly rather than at an
    // arbitrary end of the ramp
    if (range < 1e-6) return weights;

    for (std::size_t i = 0U; i < iShape3D.size(); ++i)
    {
      // Nearer is brighter; the floor keeps the far side visible instead of black
      const double t = (iShape3D[i].z - minZ) / range;
      weights[i] = 1.0 - 0.6 * t;
    }

    return weights;
  }

  std::shared_ptr<ImageMessage> Visualizer::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<UserSnapshotMessage> iUsers)
  {
    DrainCommands();

    if (!iImage || iImage->IsEmpty()) return nullptr;

    // Deep copy: assignment shares the buffer with the frame other modules read.
    cv::Mat resultImage = iImage->GetFrameBGR().clone();

    const std::size_t userCount = iUsers ? iUsers->GetSize() : 0U;

    if (userCount > 0U)
    {
      FACE_PROFILER(4_Draw);

      const auto& users = iUsers->GetUsers();

      if (mDrawGlow)
      {
        if (mGlowLayer.size() != resultImage.size() || mGlowLayer.type() != resultImage.type())
          mGlowLayer.create(resultImage.size(), resultImage.type());

        mGlowLayer.setTo(cv::Scalar::all(0.0));
      }

      // The area the glow has to be blurred over, grown so the halo is not cut off
      cv::Rect glowRoi;

      for (const auto& user : users)
      {
        if (!user) continue;

        const cv::Scalar accent = GetAccentColor(user->GetUserId());

        if (mDrawGlow)
        {
          DrawFeaturePoints(*user, mGlowLayer, accent, true);

          const cv::Rect faceRect = user->GetFaceRect();
          const int margin = (std::max)(faceRect.width, faceRect.height) / 4;
          const cv::Rect grown(faceRect.x - margin, faceRect.y - margin,
                               faceRect.width + 2 * margin, faceRect.height + 2 * margin);

          glowRoi = (glowRoi.area() > 0) ? (glowRoi | grown) : grown;
        }
      }

      if (mDrawGlow)
      {
        fw::add_glow(resultImage, mGlowLayer, glowRoi, 1.15, 25);
      }

      int slot = 0;
      for (const auto& user : users)
      {
        if (!user) continue;

        const cv::Scalar accent = GetAccentColor(user->GetUserId());

        if (mDrawPoseBox) DrawPoseBox(*user, resultImage, accent);

        DrawFeaturePoints(*user, resultImage, accent, false);

        if (mDrawPanel) DrawUserPanel(*user, resultImage, accent, slot++);
      }
    }

    DrawStatusBar(iImage, resultImage, userCount);

    // Moved in: the buffer was drawn here and is not touched afterwards, so there is no
    // reason for the message to clone it again
    return std::make_shared<ImageMessage>(std::move(resultImage), iImage->GetFrameId(), iImage->GetTimestamp());
  }

  void Visualizer::DrawFeaturePoints(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent, bool iIsGlowLayer) const
  {
    const auto& shape2D = iUser.GetShape2D();
    const auto& shape3D = iUser.GetShape3D();

    // A user that has just been detected, or whose fit failed, has no shape yet
    if (shape2D.empty()) return;

    const int pointCount = static_cast<int>(shape2D.size());
    const std::vector<double> depth = GetDepthWeights(shape3D);
    const bool hasDepth = (shape3D.size() == shape2D.size());

    for (const auto& group : sFeatureGroups)
    {
      if (group.last >= pointCount) continue;

      std::vector<cv::Point> points;
      points.reserve(group.last - group.first + 1);

      double weightSum = 0.0;
      for (int i = group.first; i <= group.last; ++i)
      {
        points.emplace_back(cvRound(shape2D[i].x), cvRound(shape2D[i].y));
        weightSum += hasDepth ? depth[i] : 1.0;
      }

      const double weight = weightSum / points.size();
      const cv::Scalar color = iAccent * weight;

      if (iIsGlowLayer)
      {
        // Thicker and plain: what matters here is the mass of light the blur spreads
        cv::polylines(oImage, points, group.closed, color, group.thickness + 2, cv::LINE_AA);
        continue;
      }

      if (group.filled)
      {
        // The eyes and the inner lip read better as surfaces than as outlines
        const cv::Rect bounds = cv::boundingRect(points) & cv::Rect(0, 0, oImage.cols, oImage.rows);
        if (bounds.area() > 0)
        {
          cv::Mat patch = oImage(bounds);
          cv::Mat tinted = patch.clone();

          std::vector<cv::Point> local;
          local.reserve(points.size());
          for (const auto& pt : points) local.emplace_back(pt.x - bounds.x, pt.y - bounds.y);

          cv::fillConvexPoly(tinted, local, color, cv::LINE_AA);
cv::addWeighted(patch, 0.78, tinted, 0.22, 0.0, patch);
        }
      }

      fw::draw_contrast_polyline(oImage, points, group.closed, color, group.thickness);
    }

    if (iIsGlowLayer) return;

    // The points themselves, small enough not to bury the curves they sit on
    for (int i = 0; i < pointCount; ++i)
    {
      const cv::Point pt(cvRound(shape2D[i].x), cvRound(shape2D[i].y));
      const double weight = hasDepth ? depth[i] : 1.0;

      cv::circle(oImage, pt, 2, cv::Scalar::all(0.0), -1, cv::LINE_AA);
      cv::circle(oImage, pt, 1, cv::Scalar::all(255.0) * weight, -1, cv::LINE_AA);
    }
  }

  void Visualizer::DrawPoseBox(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent) const
  {
    const auto& connections = PoseGeometry::GetInstance().GetConnections();

    // No pose estimated for this user yet, so there is no box to project
    if (iUser.GetFaceBox().empty() || iUser.GetRvec().empty() || iUser.GetTvec().empty() || iUser.GetCameraMatrix().empty())
      return;

    fw::VectorPt2D corners;
    fw::project_point(iUser.GetFaceBox(), iUser.GetRvec(), iUser.GetTvec(), iUser.GetCameraMatrix(), corners);

    const int cornerCount = static_cast<int>(corners.size());
    if (cornerCount == 0) return;

    const std::vector<double> depth = GetDepthWeights(iUser.GetFaceBox());
    const bool hasDepth = (depth.size() == corners.size());

    for (const auto& c : connections)
    {
      if (c.first < 0 || c.first >= cornerCount || c.second < 0 || c.second >= cornerCount)
        continue;

      // The far edges of the box are drawn fainter, which is what makes it look like a box
      const double weight = hasDepth ? (depth[c.first] + depth[c.second]) / 2.0 : 1.0;

cv::line(oImage, corners[c.first], corners[c.second], iAccent * (weight * weight * 0.4), 1, cv::LINE_AA);
    }
  }

  void Visualizer::DrawPoseGizmo(cv::Mat& oImage, const cv::Point& iCentre, int iRadius, const cv::Mat& iExtrinsics) const
  {
    if (iExtrinsics.empty() || iExtrinsics.rows < 3 || iExtrinsics.cols < 3) return;

    cv::circle(oImage, iCentre, iRadius + 4, cv::Scalar::all(235.0), 1, cv::LINE_AA);

    // Column a of the rotation is where the model's a-th axis points in camera space, so its
    // x and y are the direction on screen and its z says whether it points away from us
    struct Axis { cv::Point tip; double away; int index; };
    std::vector<Axis> axes;

    for (int a = 0; a < 3; ++a)
    {
      const double dx = iExtrinsics.at<double>(0, a);
      const double dy = iExtrinsics.at<double>(1, a);
      const double dz = iExtrinsics.at<double>(2, a);

      axes.emplace_back(Axis{
        { iCentre.x + cvRound(dx * iRadius), iCentre.y + cvRound(dy * iRadius) },
        dz, a + 1 });
    }

    // The axis pointing away is drawn first, so the near ones cross over it
    std::sort(axes.begin(), axes.end(), [](const Axis& iLhs, const Axis& iRhs) {
      return iLhs.away > iRhs.away;
    });

    for (const auto& axis : axes)
    {
      if (axis.index >= static_cast<int>(mColorsOfAxes.size())) continue;

      const double fade = axis.away > 0.0 ? 0.45 : 1.0;

      cv::line(oImage, iCentre, axis.tip, cv::Scalar::all(0.0), 3, cv::LINE_AA);
      cv::line(oImage, iCentre, axis.tip, mColorsOfAxes[axis.index] * fade, 2, cv::LINE_AA);
      cv::circle(oImage, axis.tip, 2, mColorsOfAxes[axis.index] * fade, -1, cv::LINE_AA);
    }
  }

  void Visualizer::DrawGauge(cv::Mat& oImage, const cv::Point& iTopLeft, int iWidth, int iHeight, double iValue, double iRange, const cv::Scalar& iAccent) const
  {
    const cv::Rect track(iTopLeft.x, iTopLeft.y, iWidth, iHeight);
    fw::blend_rounded_rect(oImage, track, cv::Scalar::all(210.0), iHeight / 2, 0.25);

    const int centreX = iTopLeft.x + iWidth / 2;
    const double clamped = (std::max)(-1.0, (std::min)(1.0, iRange > 0.0 ? iValue / iRange : 0.0));
    const int valueX = centreX + static_cast<int>(clamped * (iWidth / 2 - 1));

    // Grows out of the middle, so which way the head turned is visible without reading
    const int left = (std::min)(centreX, valueX);
    const int width = (std::max)(1, std::abs(valueX - centreX));

    fw::fill_rounded_rect(oImage, cv::Rect(left, iTopLeft.y, width, iHeight), iAccent, iHeight / 2);
    cv::line(oImage, { centreX, iTopLeft.y }, { centreX, iTopLeft.y + iHeight }, cv::Scalar::all(235.0), 1, cv::LINE_AA);
  }

  void Visualizer::DrawUserPanel(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent, int iSlot) const
  {
    const bool hasPose = !iUser.GetRvec().empty();

    const int width = 196;
    const int height = hasPose ? 96 : 40;
    const int margin = 10;

    // Stacked down the left edge, one panel per user, in the order they were composed
    const cv::Rect panel(margin, margin + iSlot * (height + 8), width, height);
    if ((panel & cv::Rect(0, 0, oImage.cols, oImage.rows)) != panel) return;

    fw::blend_rounded_rect(oImage, panel, cv::Scalar(28.0, 24.0, 20.0), 8, 0.62);

    // The colour chip ties the panel to the face it describes
    fw::fill_rounded_rect(oImage, cv::Rect(panel.x + 10, panel.y + 12, 6, 6), iAccent, 3);

    fw::Font title;
    title.scale = 0.42;

    std::ostringstream ss;
    ss << "USER " << iUser.GetUserId();
    fw::put_text(ss.str(), { panel.x + 24, panel.y + 19 }, oImage, false, title);

    const char* state = iUser.IsDetected() ? "DETECTED" : (iUser.IsActive() ? "TRACKED" : "LOST");
    fw::Font stateFont;
    stateFont.scale = 0.33;
    fw::put_text(state, { panel.x + 98, panel.y + 19 }, oImage, false, stateFont);

    if (!hasPose) return;

    fw::Font label;
    label.scale = 0.33;

    const cv::Vec3d& rpy = iUser.GetRPY();

    // Roll-pitch-yaw as the model stores it, shown in the order a reader expects
    const char* names[] = { "yaw", "pitch", "roll" };
    const double values[] = { fw::rad_to_deg(rpy[2]), fw::rad_to_deg(rpy[1]), fw::rad_to_deg(rpy[0]) };
    const double ranges[] = { 60.0, 40.0, 40.0 };

    for (int i = 0; i < 3; ++i)
    {
      const int rowY = panel.y + 36 + i * 17;

      fw::put_text(names[i], { panel.x + 10, rowY + 8 }, oImage, false, label);
      fw::put_text(FormatNumber(values[i], 1), { panel.x + 44, rowY + 8 }, oImage, false, label);

      DrawGauge(oImage, { panel.x + 88, rowY + 1 }, 52, 7, values[i], ranges[i], iAccent);
    }

    DrawPoseGizmo(oImage, { panel.x + 166, panel.y + 58 }, 18, iUser.GetExtrinsics());
  }

  void Visualizer::DrawStatusBar(std::shared_ptr<ImageMessage> iImage, cv::Mat& oImage, std::size_t iUserCount)
  {
    const double runtimeMs = fw::elapsed_since(iImage->GetTimestamp()).count();

    // Members, not function statics: resettable with the rest of the module.
    if (runtimeMs < mMinRuntimeMs) mMinRuntimeMs = runtimeMs;
    if (runtimeMs > mMaxRuntimeMs) mMaxRuntimeMs = runtimeMs;

    mRuntimeHistory.emplace_back(runtimeMs);
    while (mRuntimeHistory.size() > sRuntimeHistorySize) mRuntimeHistory.pop_front();

    const int barHeight = 26;
    const cv::Rect bar(0, oImage.rows - barHeight, oImage.cols, barHeight);
    fw::blend_rounded_rect(oImage, bar, cv::Scalar(24.0, 20.0, 16.0), 0, 0.58);

    std::ostringstream ss;
    ss << "frame " << iImage->GetFrameId()
       << "   " << FormatNumber(runtimeMs, 1) << " ms";

    // A frame can be processed inside the same millisecond the timestamp was taken.
    if (runtimeMs > 0.0)
      ss << "   " << cvRound(1000.0 / runtimeMs) << " fps";

    ss << "   queue " << iImage->GetQueueData().size
       << "   users " << iUserCount;

    fw::Font font;
    font.scale = 0.38;
    fw::put_text(ss.str(), { 10, oImage.rows - 9 }, oImage, false, font);

    // The frame time of the last two seconds, so a stall is visible as it happens
    if (mRuntimeHistory.size() > 2U)
    {
      const int plotWidth = (std::min)(160, oImage.cols / 3);
      const int plotHeight = barHeight - 10;
      const int plotX = oImage.cols - plotWidth - 10;
      const int plotY = oImage.rows - barHeight + 5;

      const double upper = (std::max)(1.0, *std::max_element(mRuntimeHistory.begin(), mRuntimeHistory.end()));

      std::vector<cv::Point> curve;
      curve.reserve(mRuntimeHistory.size());

      for (std::size_t i = 0U; i < mRuntimeHistory.size(); ++i)
      {
        const double t = static_cast<double>(i) / (sRuntimeHistorySize - 1);
        const double v = mRuntimeHistory[i] / upper;

        curve.emplace_back(plotX + static_cast<int>(t * plotWidth),
                           plotY + plotHeight - static_cast<int>(v * plotHeight));
      }

      cv::polylines(oImage, curve, false, fw::get_color(runtimeMs, mMinRuntimeMs, mMaxRuntimeMs), 1, cv::LINE_AA);
    }

    if (mVerboseMode) DrawProfiler(oImage, iImage->GetFrameId());
  }

  void Visualizer::DrawProfiler(cv::Mat& oImage, uint32_t iFrameId) const
  {
#ifdef ENABLE_FACE_PROFILER
    const auto& lastMeasurement = fw::ProfilerDatabase::GetInstance().GetLastMeasurement();
    if (lastMeasurement.empty()) return;

    const int rowHeight = 16;
    const int width = 210;
    const int height = static_cast<int>(lastMeasurement.size()) * rowHeight + 14;
    const cv::Rect panel(oImage.cols - width - 10, 10, width, height);

    fw::blend_rounded_rect(oImage, panel, cv::Scalar(28.0, 24.0, 20.0), 8, 0.62);

    fw::Font font;
    font.scale = 0.33;

    // The slowest stage sets the bar length, so the row that costs the most is obvious
    double slowest = 1.0;
    for (const auto& m : lastMeasurement) slowest = (std::max)(slowest, m.second.second);

    int row = 0;
    for (const auto& m : lastMeasurement)
    {
      const int rowY = panel.y + 12 + row * rowHeight;

      fw::put_text(m.first, { panel.x + 10, rowY + 8 }, oImage, false, font);
      fw::put_text(FormatNumber(m.second.second, 1), { panel.x + 150, rowY + 8 }, oImage, false, font);

      const int barWidth = (std::max)(1, static_cast<int>((m.second.second / slowest) * 130.0));
      fw::fill_rounded_rect(oImage, cv::Rect(panel.x + 10, rowY + 10, barWidth, 2), fw::get_color(m.second.second, 0.0, slowest), 1);

      ++row;
    }
#endif
  }

  std::string Visualizer::FormatNumber(double iValue, int iDecimals)
  {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(iDecimals) << iValue;
    return ss.str();
  }
}
