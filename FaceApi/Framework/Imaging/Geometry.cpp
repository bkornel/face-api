#include "Framework/Imaging/Geometry.h"

#include <algorithm>

namespace fw
{
  float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2)
  {
    if (iR1.area() == 0 || iR2.area() == 0)
      return 0.0F;

    const cv::Rect& intersection = (iR1 & iR2);
    return (std::max)(static_cast<float>(intersection.area()) / static_cast<float>(iR1.area()),
                      static_cast<float>(intersection.area()) / static_cast<float>(iR2.area()));
  }
}
