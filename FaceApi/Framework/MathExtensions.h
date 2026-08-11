#pragma once

#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>

namespace fw
{
  constexpr double deg_to_rad(double iDegree)
  {
    return iDegree * (std::numbers::pi / 180.0);
  }

  constexpr double rad_to_deg(double iRadian)
  {
    return iRadian * (180.0 / std::numbers::pi);
  }

  template <typename T>
  inline bool equals(T iA, T iB)
  {
    return std::abs(iA - iB) <= std::numeric_limits<T>::epsilon();
  }

  // Maps iValueIn from the [iBaseMin, iBaseMax] interval onto [iLimitMin, iLimitMax]
  template <typename T>
  T scale_interval(T iValueIn, T iBaseMin, T iBaseMax, T iLimitMin, T iLimitMax)
  {
    const double baseRange = static_cast<double>(iBaseMax) - static_cast<double>(iBaseMin);

    if (std::abs(baseRange) <= std::numeric_limits<double>::epsilon())
      return iLimitMin;

    const double ratio = (static_cast<double>(iValueIn) - static_cast<double>(iBaseMin)) / baseRange;

    return static_cast<T>(std::lerp(static_cast<double>(iLimitMin), static_cast<double>(iLimitMax), ratio));
  }
}
