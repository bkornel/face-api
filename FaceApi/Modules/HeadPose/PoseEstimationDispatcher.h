#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Projection.h"
#include "Framework/ErrorCode.h"

#include <opencv2/core.hpp>

namespace face
{
  class UserData;

  /// @brief Estimates the head pose of one user from its fitted 2-D shape. After Initialize()
  /// every member is read-only, so different users may be estimated concurrently.
  class PoseEstimationDispatcher
  {
    using ImagePts = fw::VectorPt2D;
    using ObjectPts = fw::VectorPt3D;

  public:
    PoseEstimationDispatcher() = default;

    PoseEstimationDispatcher(const PoseEstimationDispatcher& iOther) = delete;

    PoseEstimationDispatcher& operator=(const PoseEstimationDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    bool Estimate(UserData& ioData, const cv::Mat& iCameraMatrix) const;

  private:
    struct Pose
    {
      cv::Mat extrinsics;
      cv::Mat rvec;
      cv::Mat tvec;
      cv::Vec3d rpy;
      cv::Vec3d position;
    };

    static const ObjectPts sObjectPoints;

    Pose EstimatePose(const ImagePts& iImagePts, const cv::Mat& iCameraMatrix) const;

    ObjectPts EstimateShape3D(const cv::Mat& iExtrinsics) const;

    // The box depends only on the model and the configured offset, so it is built once
    ObjectPts CreateFaceBox() const;

    ObjectPts mFaceBox;

    bool mEstimateReprojection = false;
    double mFaceBoxOffset = 5.0;
  };
}
