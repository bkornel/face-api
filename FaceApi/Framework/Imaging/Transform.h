#pragma once

#include <opencv2/core.hpp>

namespace fw
{
  // iRotation is 90, 180 or 270 degrees, anything else leaves oOutput untouched.
  // iInput and oOutput may be the same matrix.
  void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation);
}
