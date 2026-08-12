#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Procrustes.h"
#include "Framework/ErrorCode.h"

#include <opencv2/core.hpp>

namespace face
{
  class User;

  /// @brief Aligns a user's shapes with the reference shape. After Initialize() every member
  /// is read-only, so different users may be normalized concurrently.
  class ShapeNormDispatcher
  {
  public:
    ShapeNormDispatcher() = default;

    ShapeNormDispatcher(const ShapeNormDispatcher& iOther) = delete;

    ShapeNormDispatcher& operator=(const ShapeNormDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    bool Normalize(User& ioUser) const;

  private:
    fw::VectorPt2D NormalizeShape2D(const User& iUser) const;

    fw::VectorPt3D NormalizeShape3D(const User& iUser) const;

    int mMaxIterations = 1000;
    double mEpsilon = 1e-6;
  };
}
