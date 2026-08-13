#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Model/FaceModel.h"

namespace face
{
  /// @brief What a face is doing, read off its shape, as five numbers in [0, 1].
  ///
  /// Every one of them is a ratio against the face's own size, so a user moving closer to
  /// the camera does not change any of them, and two different faces doing the same thing
  /// read the same. They are not calibrated to a person: 0 is a face at rest as the average
  /// face is at rest, not as this user is.
  struct ExpressionMetrics
  {
    double openMouth = 0.0;
    double openEyeLeft = 0.0;
    double openEyeRight = 0.0;
    double browRaise = 0.0;
    double smile = 0.0;
  };

  /// @brief The landmark as the index of the shape vectors
  inline constexpr int index_of(Landmark iLandmark)
  {
    return static_cast<int>(iLandmark);
  }

  /// @brief Distance between the outer eye corners: the scale everything else is measured
  /// against, and the least deformable span of a face. Zero when the shape is too short.
  double interocular_distance(const fw::VectorPt2D& iShape2D);

  double interocular_distance(const fw::VectorPt3D& iShape3D);

  /// @brief Reads the expression measures off a fitted shape.
  ///
  /// This lives with the model rather than in a host, because it is a measurement of the
  /// face and not a decision about how to show one: an Android panel and a Windows panel
  /// want the same five numbers, and neither should be deriving them itself.
  ExpressionMetrics measure_expression(const fw::VectorPt2D& iShape2D);
}
