#pragma once


#include "Framework/Imaging/Geometry.h"
#include <opencv2/core/core.hpp>

#include <vector>

namespace face
{
  /// @brief One axis of the pose gizmo, as it appears on screen.
  struct ProjectedAxis
  {
    /// @brief Direction on screen with the foreshortening kept, so its length is at most 1
    /// and shrinks as the axis turns towards the viewer. y grows downwards, as in an image.
    cv::Point2d direction;

    /// @brief Positive when the axis points away from the viewer, which is what decides both
    /// the drawing order and how much it is faded
    double away;

    /// @brief 0, 1 or 2 for the model's x, y and z
    int index;
  };

  /// @brief Turns a head rotation into the three axis directions to draw, sorted so that the
  /// one pointing away comes first and the near ones are drawn over it.
  ///
  /// Column a of a rotation is where the model's a-th axis points in camera space, so its x
  /// and y are already the direction on screen and its z says which way it leans - which is
  /// why a gizmo needs no camera projection.
  ///
  /// @param iRotation A 3x3 rotation, as cv::Rodrigues produces from a pose
  /// @return An empty vector when iRotation is not a usable rotation
  std::vector<ProjectedAxis> project_axes(const cv::Mat& iRotation);

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
