#pragma once

#include <cmath>
#include <cstdlib>
#include <limits>

namespace fw
{
  // Spelled out rather than taken from <cmath>: M_PI is not standard, and on MSVC it only
  // appears if _USE_MATH_DEFINES was defined before the include
  constexpr double cPi = 3.14159265358979323846;

  constexpr double deg_to_rad(double iDegree)
  {
    return iDegree * (cPi / 180.0);
  }

  constexpr double rad_to_deg(double iRadian)
  {
    return iRadian * (180.0 / cPi);
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
    const float baseRange = static_cast<float>(iBaseMax) - static_cast<float>(iBaseMin);

    if (std::fabs(baseRange) <= std::numeric_limits<float>::epsilon())
      return iLimitMin;

    const float scaled =
      (static_cast<float>(iLimitMax) - static_cast<float>(iLimitMin)) *
        (static_cast<float>(iValueIn) - static_cast<float>(iBaseMin)) / baseRange +
      static_cast<float>(iLimitMin);

    return static_cast<T>(scaled);
  }
}
