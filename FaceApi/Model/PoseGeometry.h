#pragma once


#include "Framework/Imaging/Geometry.h"
#include <opencv2/core/core.hpp>

#include <vector>

namespace face
{
  class PoseGeometry
  {
  public:
    using Connections = std::vector<std::pair<int, int>>;

    static PoseGeometry& GetInstance();

    PoseGeometry();

    inline const cv::Point3d& GetOrigin3D() const
    {
      return mOrigin3D;
    }

    inline const fw::VectorPt3D& GetAxes3D() const
    {
      return mAxes3D;
    }

    inline const fw::VectorPt3D& GetUnitBox() const
    {
      return mUnitBox;
    }

    //  E---------F
    // /|        /|
    // A--------B |
    // | G------|-H
    // |/       |/
    // C--------D
    inline const Connections& GetConnections() const
    {
      return mConnections;
    }

  private:
    ~PoseGeometry() = default;

    const cv::Point3d mOrigin3D;

    fw::VectorPt3D mAxes3D;
    fw::VectorPt3D mUnitBox;
    Connections mConnections;
  };
}
