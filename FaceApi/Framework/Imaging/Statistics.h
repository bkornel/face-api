#pragma once

#include <opencv2/core.hpp>

namespace fw
{
  // Sum of the squared elements over every channel, i.e. cv::norm(iMatrix) squared
  double sum_squared(const cv::Mat& iMatrix);
}
