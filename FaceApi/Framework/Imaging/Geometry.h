#pragma once

#include "Framework/MathExtensions.h"

#include <opencv2/core.hpp>

#include <array>
#include <vector>

namespace fw
{
  typedef std::vector<cv::Point2d> VectorPt2D;
  typedef std::vector<cv::Point3d> VectorPt3D;

  // Ratio of the intersection to the smaller of the two rectangles, 0 if either is empty
  float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2);

  /// @brief How near each point of a shape is, mapped to [iFar, 1] with the nearest at 1.
  ///
  /// Fading the far side of a shape is what makes a flat overlay read as something with a
  /// front and a back. A shape with no depth spread comes back all ones rather than at an
  /// arbitrary end of the ramp.
  std::vector<double> depth_weights(const VectorPt3D& iShape3D, double iFar = 0.4);

  /// @brief The four corners of a rectangle as bracket strokes: three points each, the
  /// middle one being the corner itself.
  ///
  /// Brackets mark the extent of something without boxing it in, and several of them
  /// overlap far more legibly than closed rectangles do.
  ///
  /// @param iArmRatio Length of an arm as a fraction of the shorter side, clamped sensibly
  std::array<std::array<cv::Point2d, 3>, 4> corner_brackets(const cv::Rect2d& iRect, double iArmRatio = 0.18);

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
