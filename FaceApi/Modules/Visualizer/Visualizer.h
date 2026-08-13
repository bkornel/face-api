#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ImageMessage.h"
#include "Messages/UserSnapshotMessage.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <opencv2/core/core.hpp>
#include <limits>
#include <string>
#include <vector>

namespace face
{
  /// @brief Draws what the API determined onto the frame.
  ///
  /// The feature points are not drawn as a uniform cloud: the layout is split into the parts
  /// of a face - jaw, brows, eyes, nose, lips - and each is stroked as its own curve, shaded
  /// by how far it lies from the camera, so a turned head reads as turned. The strokes are
  /// laid into an offscreen layer first and blurred back over the frame, which is where the
  /// glow around them comes from.
  class Visualizer : public fw::Module,
                     public fw::Port<std::shared_ptr<ImageMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<UserSnapshotMessage>)>
  {
  public:

    Visualizer() = default;

    virtual ~Visualizer() = default;

    std::shared_ptr<ImageMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<UserSnapshotMessage> iUsers) override;

    void Clear() override
    {
      mMinRuntimeMs = (std::numeric_limits<double>::max)();
      mMaxRuntimeMs = (std::numeric_limits<double>::lowest)();
      mRuntimeHistory.clear();
      mGlowLayer.release();
    }

  private:
    static const std::size_t sRuntimeHistorySize;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    /// @brief A stable colour per user, so the same face keeps its colour frame to frame
    cv::Scalar GetAccentColor(int iUserId) const;

    void DrawFeaturePoints(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent, bool iIsGlowLayer) const;

    void DrawPoseBox(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent) const;

    /// @brief A small three-axis gizmo, drawn straight from the rotation so it needs no
    /// camera projection. It lives in the panel rather than on the face, where three lines
    /// would cover the features they describe.
    void DrawPoseGizmo(cv::Mat& oImage, const cv::Point& iCentre, int iRadius, const cv::Mat& iExtrinsics) const;

    void DrawUserPanel(const User& iUser, cv::Mat& oImage, const cv::Scalar& iAccent, int iSlot) const;

    void DrawGauge(cv::Mat& oImage, const cv::Point& iTopLeft, int iWidth, int iHeight, double iValue, double iRange, const cv::Scalar& iAccent) const;

    void DrawStatusBar(std::shared_ptr<ImageMessage> iImage, cv::Mat& oImage, std::size_t iUserCount);

    void DrawProfiler(cv::Mat& oImage, uint32_t iFrameId) const;

    static std::string FormatNumber(double iValue, int iDecimals);

    std::vector<cv::Scalar> mColorsOfAxes;
    std::vector<cv::Scalar> mAccentPalette;

    /// @brief Where the strokes are drawn before they are blurred back over the frame
    cv::Mat mGlowLayer;

    std::deque<double> mRuntimeHistory;

    double mMinRuntimeMs = (std::numeric_limits<double>::max)();
    double mMaxRuntimeMs = (std::numeric_limits<double>::lowest)();

    bool mDrawGlow = true;
    bool mDrawPanel = true;
    bool mDrawPoseBox = true;
  };
}
