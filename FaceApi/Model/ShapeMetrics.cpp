#include "Model/ShapeMetrics.h"

#include <algorithm>
#include <cmath>

namespace face
{
  namespace
  {
    /// @brief The number of points a fitted shape has, and therefore the shortest one any of
    /// these measures can be read from
    constexpr std::size_t sShapeSize = 68U;

    /// @brief Maps iValue from [iRest, iExtreme] onto [0, 1], clamped at both ends. iExtreme
    /// may be below iRest, which is how a measure that shrinks as it opens is handled.
    double normalise(double iValue, double iRest, double iExtreme)
    {
      const double span = iExtreme - iRest;
      if (std::abs(span) < 1e-9) return 0.0;

      return (std::max)(0.0, (std::min)(1.0, (iValue - iRest) / span));
    }

    double distance(const cv::Point2d& iA, const cv::Point2d& iB)
    {
      return cv::norm(iA - iB);
    }

    /// @brief The eye aspect ratio of one eye, given the first of its six points.
    ///
    /// Two vertical spans over the horizontal one. It is the standard measure because it
    /// survives the head turning: all three distances shrink together with the foreshortening.
    double eye_aspect_ratio(const fw::VectorPt2D& iShape2D, int iFirst)
    {
      const double width = distance(iShape2D[iFirst], iShape2D[iFirst + 3]);
      if (width < 1e-6) return 0.0;

      const double upper = distance(iShape2D[iFirst + 1], iShape2D[iFirst + 5]);
      const double lower = distance(iShape2D[iFirst + 2], iShape2D[iFirst + 4]);

      return (upper + lower) / (2.0 * width);
    }

    cv::Point2d centroid(const fw::VectorPt2D& iShape2D, Landmark iFirst, Landmark iLast)
    {
      const int first = index_of(iFirst);
      const int last = index_of(iLast);

      cv::Point2d sum(0.0, 0.0);
      for (int i = first; i <= last; ++i) sum += iShape2D[i];

      return sum / static_cast<double>(last - first + 1);
    }
  }

  double interocular_distance(const fw::VectorPt2D& iShape2D)
  {
    if (iShape2D.size() < sShapeSize) return 0.0;

    return distance(iShape2D[index_of(Landmark::kRightEye0)], iShape2D[index_of(Landmark::kLeftEye3)]);
  }

  double interocular_distance(const fw::VectorPt3D& iShape3D)
  {
    if (iShape3D.size() < sShapeSize) return 0.0;

    return cv::norm(iShape3D[index_of(Landmark::kRightEye0)] - iShape3D[index_of(Landmark::kLeftEye3)]);
  }

  ExpressionMetrics measure_expression(const fw::VectorPt2D& iShape2D)
  {
    ExpressionMetrics metrics;

    if (iShape2D.size() < sShapeSize) return metrics;

    const double scale = interocular_distance(iShape2D);
    if (scale < 1e-6) return metrics;

    // Eyes: about 0.31 wide open, about 0.10 shut. Below the closed end the measure is
    // noise, above the open end it is a stare, and both clamp.
    metrics.openEyeRight = normalise(eye_aspect_ratio(iShape2D, index_of(Landmark::kRightEye0)), 0.10, 0.31);
    metrics.openEyeLeft = normalise(eye_aspect_ratio(iShape2D, index_of(Landmark::kLeftEye0)), 0.10, 0.31);

    // Mouth: the inner lip contour, because the outer one barely moves when the jaw drops.
    // Three vertical spans averaged, over the width, so a smile does not read as an opening.
    const int innerFirst = index_of(Landmark::kMouth12);
    const double mouthWidth = distance(iShape2D[innerFirst], iShape2D[innerFirst + 4]);

    if (mouthWidth > 1e-6)
    {
      const double opening = (distance(iShape2D[innerFirst + 1], iShape2D[innerFirst + 7]) +
                              distance(iShape2D[innerFirst + 2], iShape2D[innerFirst + 6]) +
                              distance(iShape2D[innerFirst + 3], iShape2D[innerFirst + 5])) / 3.0;

      metrics.openMouth = normalise(opening / mouthWidth, 0.06, 0.62);
    }

    // Brows: how far a brow sits above the eye it belongs to, in units of the face's own
    // width. The two are averaged, since one brow alone is more often a fit error than a
    // raised eyebrow.
    const cv::Point2d rightBrow = centroid(iShape2D, Landmark::kRightEyebrow0, Landmark::kRightEyebrow4);
    const cv::Point2d leftBrow = centroid(iShape2D, Landmark::kLeftEyebrow0, Landmark::kLeftEyebrow4);
    const cv::Point2d rightEye = centroid(iShape2D, Landmark::kRightEye0, Landmark::kRightEye5);
    const cv::Point2d leftEye = centroid(iShape2D, Landmark::kLeftEye0, Landmark::kLeftEye5);

    const double browGap = (distance(rightBrow, rightEye) + distance(leftBrow, leftEye)) / (2.0 * scale);
    metrics.browRaise = normalise(browGap, 0.22, 0.38);

    // Smile: the corners travel outwards and upwards. Width alone reads a wide face as a
    // smile, so it is paired with how high the corners sit against the lip centre line.
    const cv::Point2d rightCorner = iShape2D[index_of(Landmark::kMouth0)];
    const cv::Point2d leftCorner = iShape2D[index_of(Landmark::kMouth6)];

    const double cornerWidth = distance(rightCorner, leftCorner) / scale;

    const double cornerY = (rightCorner.y + leftCorner.y) * 0.5;
    const double lipCentreY = (iShape2D[index_of(Landmark::kMouth3)].y +
                               iShape2D[index_of(Landmark::kMouth9)].y) * 0.5;

    // y grows downwards, so a corner above the centre line is a positive lift
    const double lift = (lipCentreY - cornerY) / scale;

    metrics.smile = 0.65 * normalise(cornerWidth, 0.62, 0.95) +
                    0.35 * normalise(lift, -0.02, 0.09);

    return metrics;
  }
}
