#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Procrustes.h"
#include "Framework/ErrorCode.h"

#include <opencv2/core.hpp>

namespace face
{
  /// @brief Aligns a shape with the reference shape. After Initialize() every member is
  /// read-only, so different faces may be normalized concurrently.
  class ShapeNormDispatcher
  {
  public:
    ShapeNormDispatcher() = default;

    ShapeNormDispatcher(const ShapeNormDispatcher& iOther) = delete;

    ShapeNormDispatcher& operator=(const ShapeNormDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    fw::VectorPt2D Normalize2D(const fw::VectorPt2D& iShape2D) const;

    fw::VectorPt3D Normalize3D(const fw::VectorPt3D& iShape3D) const;

  private:
    int mMaxIterations = 1000;
    double mEpsilon = 1e-6;
  };
}
