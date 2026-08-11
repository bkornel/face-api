#pragma once

#include "Framework/Ocv/Geometry.h"

#include <opencv2/core.hpp>

namespace fw
{
  namespace ocv
  {
    void project_point(const cv::Point3d& iPoint3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, cv::Point2d& oPoint2D);

    // oPoints2D comes back empty if iPoints3D is empty, so callers must check before indexing
    void project_point(const VectorPt3D& iPoints3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, VectorPt2D& oPoints2D);

    // Intrinsics guessed from the frame size and an assumed 70 degree diagonal field of view
    cv::Mat get_camera_matrix(const cv::Size& iSize);
  }
}
