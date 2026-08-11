#pragma once

#include <opencv2/core.hpp>

namespace fw
{
  // iRotation is 90, 180 or 270 degrees, anything else leaves oOutput untouched
  void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation);

  // Sum of the squared elements over every channel
  double sum_squared(const cv::Mat& iMatrix);
}
