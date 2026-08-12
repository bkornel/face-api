#pragma once


#include "Framework/Imaging/Geometry.h"
#include <opencv2/core/core.hpp>

#include <map>
#include <vector>

namespace face
{
  enum class ShapeType
  {
    kOriginalShape = 0,
    kNormalizedShape
  };

  enum class BodyPart
  {
    kContour = 0,
    kLeftEye,
    kLeftUpperEyelid,
    kLeftLowerEyelid,
    kRightEye,
    kRightUpperEyelid,
    kRightLowerEyelid,
    kRightEyebrow,
    kLeftEyebrow,
    kNose,
    kUpperLip,
    kLowerLip,
    kLeftLip,
    kRightLip,
    kUndefined
  };

  enum class Landmark
  {
    kContour0 = 0,
    kContour1,
    kContour2,
    kContour3,
    kContour4,
    kContour5,
    kContour6,
    kContour7,
    kContour8,
    kContour9,
    kContour10,
    kContour11,
    kContour12,
    kContour13,
    kContour14,
    kContour15,
    kContour16,

    kRightEyebrow0,
    kRightEyebrow1,
    kRightEyebrow2,
    kRightEyebrow3,
    kRightEyebrow4,

    kLeftEyebrow0,
    kLeftEyebrow1,
    kLeftEyebrow2,
    kLeftEyebrow3,
    kLeftEyebrow4,

    kNose0,
    kNose1,
    kNose2,
    kNose3,
    kNose4,
    kNose5,
    kNose6,
    kNose7,
    kNose8,

    kRightEye0,
    kRightEye1,
    kRightEye2,
    kRightEye3,
    kRightEye4,
    kRightEye5,

    kLeftEye0,
    kLeftEye1,
    kLeftEye2,
    kLeftEye3,
    kLeftEye4,
    kLeftEye5,

    kMouth0,
    kMouth1,
    kMouth2,
    kMouth3,
    kMouth4,
    kMouth5,
    kMouth6,
    kMouth7,
    kMouth8,
    kMouth9,
    kMouth10,
    kMouth11,
    kMouth12,
    kMouth13,
    kMouth14,
    kMouth15,
    kMouth16,
    kMouth17
  };

  class FaceModel
  {
  public:
    using Landmarks = std::vector<Landmark>;
    using ShapeParts = std::map<BodyPart, Landmarks>;
    using Connections = std::vector<std::pair<int, int>>;

    static FaceModel& GetInstance();

    FaceModel();

    const Landmark sFixPointID = Landmark::kNose6;

    const std::string& PointNameToString(Landmark iLandmark) const;

    const std::string& ShapeClusterToString(BodyPart iShapeCluster) const;

    BodyPart GetShapeClusterID(Landmark iLandmark) const;

    const Landmarks& GetLandmarks(BodyPart iClusterId) const;

    inline const Connections& GetConnections() const
    {
      return mConnections;
    }

    inline const fw::VectorPt3D& GetShape3D() const
    {
      return mShape3D;
    }

    /// @brief The frontal view of the model: its x and y, which the model lays out in image
    /// orientation. This is what shapes are aligned against when they are normalized.
    const fw::VectorPt2D& GetFrontalShape2D() const
    {
      return mFrontalShape2D;
    }

  private:
    ~FaceModel() = default;

    ShapeParts mShapeParts;
    Connections mConnections;
    fw::VectorPt3D mShape3D;
    fw::VectorPt2D mFrontalShape2D;

    std::map<Landmark, std::string> mShapePoints;
    std::map<BodyPart, std::string> mShapeClusters;
  };
}
