#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Projection.h"
#include "Framework/ErrorCode.h"

#include "Messages/PoseMessage.h"
#include "Messages/ShapeMessage.h"

#include <opencv2/core.hpp>

namespace face
{
  /// @brief Estimates the 6DoF head pose belonging to one fitted shape. After Initialize()
  /// every member is read-only, so different faces may be estimated concurrently.
  class PoseEstimationDispatcher
  {
    using ImagePts = fw::VectorPt2D;
    using ObjectPts = fw::VectorPt3D;

  public:
    PoseEstimationDispatcher() = default;

    PoseEstimationDispatcher(const PoseEstimationDispatcher& iOther) = delete;

    PoseEstimationDispatcher& operator=(const PoseEstimationDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    bool Estimate(const ShapeDescriptor& iShape, const cv::Mat& iCameraMatrix, PoseDescriptor& oPose) const;

  private:
    static const ObjectPts sObjectPoints;

    ObjectPts EstimateShape3D(const cv::Mat& iExtrinsics) const;

    // The box depends only on the model and the configured offset, so it is built once
    ObjectPts CreateFaceBox() const;

    ObjectPts mFaceBox;

    bool mEstimateReprojection = false;
    double mFaceBoxOffset = 5.0;
  };
}
