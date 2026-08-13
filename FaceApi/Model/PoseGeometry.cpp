#include "Model/PoseGeometry.h"

#include <algorithm>

namespace face
{
  std::vector<ProjectedAxis> project_axes(const cv::Mat& iRotation)
  {
    std::vector<ProjectedAxis> axes;

    if (iRotation.empty() || iRotation.rows < 3 || iRotation.cols < 3) return axes;

    cv::Mat rotation;
    iRotation.convertTo(rotation, CV_64F);

    axes.reserve(3U);

    for (int a = 0; a < 3; ++a)
    {
      axes.emplace_back(ProjectedAxis{
        cv::Point2d(rotation.at<double>(0, a), rotation.at<double>(1, a)),
        rotation.at<double>(2, a),
        a });
    }

    // The axis pointing away is first, so the near ones are drawn over it
    std::sort(axes.begin(), axes.end(), [](const ProjectedAxis& iLhs, const ProjectedAxis& iRhs) {
      return iLhs.away > iRhs.away;
    });

    return axes;
  }

  PoseGeometry& PoseGeometry::GetInstance()
  {
    static PoseGeometry sInstance;
    return sInstance;
  }

  PoseGeometry::PoseGeometry() :
    mOrigin3D(0.0, 0.0, 0.0)
  {
    mAxes3D = {
      mOrigin3D,
      { 100, 0, 0 },
      { 0, 100, 0 },
      { 0, 0, 100 }
    };

    mUnitBox = {
      // Front face
      { 0.0, 0.0, 0.0 }, // A
      { 1.0, 0.0, 0.0 }, // B
      { 0.0, 1.0, 0.0 }, // C
      { 1.0, 1.0, 0.0 }, // D

      // Rear face
      { 0.0, 0.0, 1.0 }, // E
      { 1.0, 0.0, 1.0 }, // F
      { 0.0, 1.0, 1.0 }, // G
      { 1.0, 1.0, 1.0 }  // H
    };

    mConnections = {
      // Front face
      { 0, 1 },
      { 1, 3 },
      { 3, 2 },
      { 2, 0 },

      // Rear face
      { 4, 5 },
      { 5, 7 },
      { 7, 6 },
      { 6, 4 },

      // Sides
      { 0, 4 },
      { 1, 5 },
      { 2, 6 },
      { 3, 7 }
    };
  }
}
