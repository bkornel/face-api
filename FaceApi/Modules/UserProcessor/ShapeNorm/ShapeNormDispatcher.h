#pragma once

#include "Framework/UtilOCV.h"
#include "User/UserDispatcher.hpp"

namespace face
{
  class ShapeNormDispatcher :
    public UserDispatcher
  {
    typedef std::vector<cv::Mat> MatVector;

  public:
    ShapeNormDispatcher() = default;

    virtual ~ShapeNormDispatcher() = default;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

    bool Dispatch(User& ioUser) override;

  private:
    void NormalizeShape2D(User& ioUser);

    void NormalizeShape3D(User& ioUser);

    void GPA(MatVector& ioShapes2D, cv::Mat& ioMeanShape) const;

    void Recenter(MatVector& ioShapes) const;

    void Normalize(MatVector& ioShapes) const;

    void Align(MatVector& ioShapes, cv::Mat& ioMeanShape) const;

    fw::ocv::VectorPt2D mMeanShape2D;
    fw::ocv::VectorPt3D mMeanShape3D;

    int mMaxCount = 1000;
    double mEpsilon = 1e-6;
  };
}
