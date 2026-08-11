#pragma once

#include <opencv2/core.hpp>

#include <vector>

namespace fw
{
  namespace ocv
  {
    typedef std::vector<cv::Point2d> VectorPt2D;
    typedef std::vector<cv::Point3d> VectorPt3D;

    // Ratio of the intersection to the smaller of the two rectangles, 0 if either is empty
    float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2);

    // iRotation is 90, 180 or 270 degrees, anything else leaves oOutput untouched
    void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation);
  }
}
