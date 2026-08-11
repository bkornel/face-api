#pragma once

#include "Framework/MathExtensions.h"

#include <opencv2/core.hpp>

#include <vector>

namespace fw
{
  typedef std::vector<cv::Point2d> VectorPt2D;
  typedef std::vector<cv::Point3d> VectorPt3D;

  // Ratio of the intersection to the smaller of the two rectangles, 0 if either is empty
  float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2);

  template <typename _Tp>
  inline bool equals(const cv::Rect_<_Tp>& iA, const cv::Rect_<_Tp>& iB)
  {
    return equals(iA.x, iB.x) && equals(iA.y, iB.y) &&
           equals(iA.width, iB.width) && equals(iA.height, iB.height);
  }

  // Scales around the centre. Returns iRect unchanged if the result would be empty.
  template <typename _Tp>
  inline cv::Rect_<_Tp> scale_rect(const cv::Rect_<_Tp>& iRect, float iScale)
  {
    cv::Rect_<float> fr = iRect;

    fr += cv::Point2f((1.0F - iScale) * iRect.width / 2.0F, (1.0F - iScale) * iRect.height / 2.0F);
    fr.width *= iScale;
    fr.height *= iScale;

    const cv::Rect_<_Tp> scaled(static_cast<_Tp>(fr.x), static_cast<_Tp>(fr.y),
                                static_cast<_Tp>(fr.width), static_cast<_Tp>(fr.height));

    return scaled.area() <= 0 ? iRect : scaled;
  }
}
