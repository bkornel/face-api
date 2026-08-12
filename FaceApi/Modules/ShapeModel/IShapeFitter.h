#pragma once

#include "Framework/ErrorCode.h"
#include "Messages/ShapeMessage.h"
#include "User/TrackedFace.h"

#include <opencv2/core.hpp>

#include <vector>

namespace face
{
  /// @brief A method that fits the facial feature points of one tracked face. BeginFrame()
  /// runs once per frame on one thread and maintains whatever per-track state the method
  /// keeps; Fit() must touch only the state of the track it is given, so different tracks
  /// may be fitted concurrently.
  class IShapeFitter
  {
  public:
    IShapeFitter() = default;

    IShapeFitter(const IShapeFitter& iOther) = delete;

    IShapeFitter& operator=(const IShapeFitter& iOther) = delete;

    virtual ~IShapeFitter() = default;

    virtual fw::ErrorCode Initialize(const cv::FileNode& iSettings) = 0;

    virtual void BeginFrame(const std::vector<TrackedFace>& iTracks) = 0;

    virtual bool Fit(const TrackedFace& iTrack, const cv::Mat& iFrameGray, const cv::Mat& iFrameBGR, ShapeDescriptor& oShape) = 0;

    virtual void Clear() = 0;
  };
}
