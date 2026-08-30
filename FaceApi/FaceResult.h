#pragma once


#include "Framework/Imaging/Geometry.h"
#include "Model/ShapeMetrics.h"
#include "User/TrackedFace.h"

#include <opencv2/core/core.hpp>

#include <cstdint>
#include <vector>

namespace face
{
  /// @brief Everything the API determined about one face in one frame, as plain data.
  ///
  /// This is the output to use when the host application draws the overlay itself, with
  /// its own canvas or GPU surface, instead of asking the API for a rendered image. The
  /// numbers are in the coordinate system of the frame that was pushed in.
  ///
  /// It reports what the pipeline worked out, all of it: a host that had to derive the
  /// identity's state by watching ids from frame to frame, or normalise a shape the
  /// pipeline had already normalised, would be deriving it worse and once per platform.
  struct FaceResult
  {
    int userId = 0;
    uint32_t frameId = 0U;
    int64_t timestamp = 0LL;

    /// @brief Whether this identity was detected on this frame, carried over by the tracker,
    /// or has gone inactive
    TrackStatus status = TrackStatus::Detected;

    /// @brief Seconds since the tracker first saw this identity
    double ageSeconds = 0.0;

    /// @brief Face rectangle in 2-D pixel coordinates
    cv::Rect faceRect;

    /// @brief Facial feature points in 2-D pixel coordinates
    fw::VectorPt2D shape2D;

    /// @brief Facial feature points in the 3-D camera coordinate system: the shape the
    /// fitter reconstructed, moved to where the head is
    fw::VectorPt3D shape3D;

    /// @brief The shapes with their position, scale and rotation removed, aligned onto the
    /// reference. What is left is the form of this face and what it is doing, which is what
    /// makes two faces comparable and what a driven mesh wants. Empty when the graph has no
    /// shapeNorm module, and the 3-D half also needs a pose.
    fw::VectorPt2D normShape2D;
    fw::VectorPt3D normShape3D;

    /// @brief The 8 corners of the face box in the 3-D camera coordinate system
    fw::VectorPt3D faceBox;

    /// @brief Head orientation as roll-pitch-yaw in radians
    cv::Vec3d rpy;

    /// @brief Head position in the 3-D camera coordinate system
    cv::Vec3d position3D;

    /// @brief What the face is doing, as five scale-free measures in [0, 1]
    ExpressionMetrics expression;

    /// @brief Needed to project the 3-D data back to the image, e.g. cv::projectPoints
    cv::Mat cameraMatrix;
    cv::Mat rvec;
    cv::Mat tvec;

    /// @brief Whether the pose was solved for this face. Without it the angles, the position
    /// and the face box are stale, and nothing can be projected.
    inline bool HasPose() const
    {
      return !rvec.empty() && !tvec.empty() && !cameraMatrix.empty();
    }
  };

  using FaceResults = std::vector<FaceResult>;
}
