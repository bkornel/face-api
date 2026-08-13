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

namespace fw
{
  std::vector<double> depth_weights(const VectorPt3D& iShape3D, double iFar)
  {
    std::vector<double> weights(iShape3D.size(), 1.0);

    if (iShape3D.empty()) return weights;

    double minZ = iShape3D[0].z;
    double maxZ = iShape3D[0].z;

    for (const auto& point : iShape3D)
    {
      minZ = (std::min)(minZ, point.z);
      maxZ = (std::max)(maxZ, point.z);
    }

    const double range = maxZ - minZ;

    // A flat shape carries no depth to fade by
    if (range < 1e-6) return weights;

    const double floor = (std::max)(0.0, (std::min)(1.0, iFar));

    for (std::size_t i = 0U; i < iShape3D.size(); ++i)
    {
      const double t = (iShape3D[i].z - minZ) / range;
      weights[i] = 1.0 - (1.0 - floor) * t;
    }

    return weights;
  }

  std::array<std::array<cv::Point2d, 3>, 4> corner_brackets(const cv::Rect2d& iRect, double iArmRatio)
  {
    const double left = iRect.x;
    const double top = iRect.y;
    const double right = iRect.x + iRect.width;
    const double bottom = iRect.y + iRect.height;

    const double shorter = (std::min)(std::abs(iRect.width), std::abs(iRect.height));
    const double arm = (std::max)(1.0, shorter * (std::max)(0.02, (std::min)(0.45, iArmRatio)));

    return { {
      { { cv::Point2d(left, top + arm), cv::Point2d(left, top), cv::Point2d(left + arm, top) } },
      { { cv::Point2d(right - arm, top), cv::Point2d(right, top), cv::Point2d(right, top + arm) } },
      { { cv::Point2d(right, bottom - arm), cv::Point2d(right, bottom), cv::Point2d(right - arm, bottom) } },
      { { cv::Point2d(left + arm, bottom), cv::Point2d(left, bottom), cv::Point2d(left, bottom - arm) } }
    } };
  }
}
