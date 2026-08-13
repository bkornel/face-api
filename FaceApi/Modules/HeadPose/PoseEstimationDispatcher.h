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

    /// @brief Moves a shape from the model's space to the camera's
    ObjectPts EstimateShape3D(const cv::Mat& iExtrinsics, const ObjectPts& iShape) const;

    /// @brief Where the head must be for a known rotation of it to project onto the tracked
    /// landmarks. Linear in the translation and solved in the least-squares sense over all
    /// sixty-eight of them, which is why it needs no starting guess and cannot land on the
    /// wrong one of two answers the way a full 6DoF solve can.
    /// @return false if the correspondences are degenerate, leaving oTvec untouched
    bool SolveTranslation(const cv::Matx33d& iRotation, const ObjectPts& iObjectPts,
                          const ImagePts& iImagePts, const cv::Mat& iCameraMatrix,
                          cv::Mat& oTvec) const;

    // The box depends only on the model and the configured offset, so it is built once
    ObjectPts CreateFaceBox() const;

    ObjectPts mFaceBox;

    bool mEstimateReprojection = false;
    double mFaceBoxOffset = 5.0;

    /// @brief Whether the rotation is taken from the shape model rather than solved for.
    ///
    /// Measured on the sample clips, where the face translates across the frame and never
    /// turns: the shape model reports a yaw that stays inside 1.3 degrees, while solving the
    /// pose from the landmarks against the canonical model invents a yaw wandering over 15
    /// degrees on one clip and 31 on the other - which is the head seen to swing about while
    /// its owner sits still. Seeding the solver with the regressed rotation does not help;
    /// it converges to the same wrong place from any starting point, because the minimum
    /// itself is in the wrong place. The canonical model is not this face, and no rigid
    /// motion of it fits the landmarks well enough for the best one to be meaningful.
    ///
    /// Turn it off to go back to solving the whole pose, for a shape model that regresses no
    /// rotation of its own - it is used only when one is offered.
    bool mPoseFromShapeModel = true;
  };
}
