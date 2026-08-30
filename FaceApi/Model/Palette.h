#pragma once

#include <opencv2/core/core.hpp>

#include <array>
#include <cstddef>

namespace face
{
  /// @brief The colours every renderer draws a user with.
  ///
  /// One table for all of them: the pipeline's Visualizer, the Direct2D and Direct3D views
  /// and the UI's panels used to carry their own copies with a "keep in step" comment, and
  /// a colour edited in one of them quietly stopped matching the rest. A consumer converts
  /// at its own boundary - BGR for OpenCV, floats for a shader, ARGB for XAML.
  namespace palette
  {
    /// @brief One colour, as red, green and blue in [0, 1]
    struct Rgb
    {
      double r;
      double g;
      double b;
    };

    /// @brief The per-user accents, picked to stay apart from one another and from skin tones
    inline constexpr std::array<Rgb, 5> kAccents = { {
      { 0.30, 0.78, 1.00 },  // sky
      { 0.55, 0.94, 0.60 },  // mint
      { 1.00, 0.73, 0.35 },  // amber
      { 0.94, 0.55, 0.80 },  // orchid
      { 1.00, 0.47, 0.47 }   // coral
    } };

    /// @brief The model's x, y and z axes, in that order
    inline constexpr std::array<Rgb, 3> kAxes = { {
      { 1.00, 0.42, 0.42 },
      { 0.45, 0.90, 0.50 },
      { 0.40, 0.70, 1.00 }
    } };

    /// @brief A stable colour per user, so the same face keeps its colour frame to frame
    inline const Rgb& accent_of(int iUserId)
    {
      const auto index = static_cast<std::size_t>(iUserId < 0 ? -static_cast<long long>(iUserId) : iUserId);
      return kAccents[index % kAccents.size()];
    }

    /// @brief The colour as OpenCV's drawing calls want it: blue, green, red in [0, 255]
    inline cv::Scalar to_bgr(const Rgb& iColor)
    {
      return { iColor.b * 255.0, iColor.g * 255.0, iColor.r * 255.0 };
    }
  }
}
