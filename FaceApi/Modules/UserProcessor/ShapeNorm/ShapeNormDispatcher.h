#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Procrustes.h"
#include "Framework/ErrorCode.h"
#include "User/UserDispatcher.hpp"

namespace face
{
  class ShapeNormDispatcher : public UserDispatcher
  {
  public:
    ShapeNormDispatcher() = default;

    virtual ~ShapeNormDispatcher() = default;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

    bool Dispatch(User& ioUser) override;

  private:
    void NormalizeShape2D(User& ioUser);

    void NormalizeShape3D(User& ioUser);

    fw::VectorPt2D mMeanShape2D;
    fw::VectorPt3D mMeanShape3D;

    int mMaxIterations = 1000;
    double mEpsilon = 1e-6;
  };
}
