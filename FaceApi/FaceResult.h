#pragma once

#include "Framework/UtilOCV.h"

#include <opencv2/core/core.hpp>

#include <vector>

namespace face
{
  /// @brief Everything the API determined about one face in one frame, as plain data.
  ///
  /// This is the output to use when the host application draws the overlay itself, with
  /// its own canvas or GPU surface, instead of asking the API for a rendered image. The
  /// numbers are in the coordinate system of the frame that was pushed in.
  struct FaceResult
  {
    int userId = 0;
    unsigned frameId = 0U;
    long long timestamp = 0LL;

    /// @brief Face rectangle in 2-D pixel coordinates
    cv::Rect faceRect;

    /// @brief Facial feature points in 2-D pixel coordinates
    fw::ocv::VectorPt2D shape2D;

    /// @brief Facial feature points in the 3-D camera coordinate system
    fw::ocv::VectorPt3D shape3D;

    /// @brief The 8 corners of the face box in the 3-D camera coordinate system
    fw::ocv::VectorPt3D faceBox;

    /// @brief Head orientation as roll-pitch-yaw in radians
    cv::Vec3d rpy;

    /// @brief Head position in the 3-D camera coordinate system
    cv::Vec3d position3D;

    /// @brief Needed to project the 3-D data back to the image, e.g. cv::projectPoints
    cv::Mat cameraMatrix;
    cv::Mat rvec;
    cv::Mat tvec;
  };

  using FaceResults = std::vector<FaceResult>;
}
