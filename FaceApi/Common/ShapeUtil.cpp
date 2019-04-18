#include "Common/ShapeUtil.h"

namespace face
{
  ShapeUtil& ShapeUtil::GetInstance()
  {
    static ShapeUtil sInstance;
    return sInstance;
  }

  ShapeUtil::ShapeUtil()
  {
    mShapeParts = {
      { BodyPart::kContour,
      { Landmark::kContour0, Landmark::kContour1, Landmark::kContour2, Landmark::kContour3, Landmark::kContour4, Landmark::kContour5, Landmark::kContour6, Landmark::kContour7, Landmark::kContour8, Landmark::kContour9, Landmark::kContour10, Landmark::kContour11, Landmark::kContour12, Landmark::kContour13, Landmark::kContour14, Landmark::kContour15, Landmark::kContour16 } },
      { BodyPart::kNose,
      { Landmark::kNose0, Landmark::kNose1, Landmark::kNose2, Landmark::kNose3, Landmark::kNose4, Landmark::kNose5, Landmark::kNose6, Landmark::kNose7, Landmark::kNose8 } },
      { BodyPart::kLeftEye,
      { Landmark::kLeftEye1, Landmark::kLeftEye2, Landmark::kLeftEye3, Landmark::kLeftEye4, Landmark::kLeftEye5 } },
      { BodyPart::kLeftEyebrow,
      { Landmark::kLeftEyebrow0, Landmark::kLeftEyebrow1, Landmark::kLeftEyebrow2, Landmark::kLeftEyebrow3, Landmark::kLeftEyebrow4 } },
      { BodyPart::kLeftUpperEyelid,
      { Landmark::kLeftEye1, Landmark::kLeftEye2 } },
      { BodyPart::kLeftLowerEyelid,
      { Landmark::kLeftEye4, Landmark::kLeftEye5 } },
      { BodyPart::kRightEye,
      { Landmark::kRightEye1, Landmark::kRightEye2, Landmark::kRightEye3, Landmark::kRightEye4, Landmark::kRightEye5 } },
      { BodyPart::kRightEyebrow,
      { Landmark::kRightEyebrow0, Landmark::kRightEyebrow1, Landmark::kRightEyebrow2, Landmark::kRightEyebrow3, Landmark::kRightEyebrow4 } },
      { BodyPart::kRightUpperEyelid,
      { Landmark::kRightEye1, Landmark::kRightEye2 } },
      { BodyPart::kRightLowerEyelid,
      { Landmark::kRightEye4, Landmark::kRightEye5 } },
      { BodyPart::kUpperLip,
      { Landmark::kMouth2, Landmark::kMouth3, Landmark::kMouth4 } },
      { BodyPart::kLowerLip,
      { Landmark::kMouth8, Landmark::kMouth9, Landmark::kMouth10 } },
      { BodyPart::kLeftLip,
      { Landmark::kMouth6 } },
      { BodyPart::kRightLip,
      { Landmark::kMouth0 } }
    };

    mShapePoints = {
      { Landmark::kContour0, "CONTOUR_0" },{ Landmark::kContour1, "CONTOUR_1" },{ Landmark::kContour2, "CONTOUR_2" },{ Landmark::kContour3, "CONTOUR_3" },
      { Landmark::kContour4, "CONTOUR_4" },{ Landmark::kContour5, "CONTOUR_5" },{ Landmark::kContour6, "CONTOUR_6" },{ Landmark::kContour7, "CONTOUR_7" },
      { Landmark::kContour8, "CONTOUR_8" },{ Landmark::kContour9, "CONTOUR_9" },{ Landmark::kContour10, "CONTOUR_10" },{ Landmark::kContour11, "CONTOUR_11" },
      { Landmark::kContour12, "CONTOUR_12" },{ Landmark::kContour13, "CONTOUR_13" },{ Landmark::kContour14, "CONTOUR_14" },{ Landmark::kContour15, "CONTOUR_15" },
      { Landmark::kContour16, "CONTOUR_16" },
      { Landmark::kRightEyebrow0, "RIGHT_EYE_BROW_0" },{ Landmark::kRightEyebrow1, "RIGHT_EYE_BROW_1" },{ Landmark::kRightEyebrow2, "RIGHT_EYE_BROW_2" },
      { Landmark::kRightEyebrow3, "RIGHT_EYE_BROW_3" },{ Landmark::kRightEyebrow4, "RIGHT_EYE_BROW_4" },
      { Landmark::kLeftEyebrow0, "LEFT_EYE_BROW_0" },{ Landmark::kLeftEyebrow1, "LEFT_EYE_BROW_1" },{ Landmark::kLeftEyebrow2, "LEFT_EYE_BROW_2" },
      { Landmark::kLeftEyebrow3, "LEFT_EYE_BROW_3" },{ Landmark::kLeftEyebrow4, "LEFT_EYE_BROW_4" },
      { Landmark::kNose0, "NOSE_0" },{ Landmark::kNose1, "NOSE_1" },{ Landmark::kNose2, "NOSE_2" },{ Landmark::kNose3, "NOSE_3" },{ Landmark::kNose4, "NOSE_4" },
      { Landmark::kNose5, "NOSE_5" },{ Landmark::kNose6, "NOSE_6" },{ Landmark::kNose7, "NOSE_7" },{ Landmark::kNose8, "NOSE_8" },
      { Landmark::kRightEye0, "RIGHT_EYE_0" },{ Landmark::kRightEye1, "RIGHT_EYE_1" },{ Landmark::kRightEye2, "RIGHT_EYE_2" },{ Landmark::kRightEye3, "RIGHT_EYE_3" },
      { Landmark::kRightEye4, "RIGHT_EYE_4" },{ Landmark::kRightEye5, "RIGHT_EYE_5" },
      { Landmark::kLeftEye0, "LEFT_EYE_0" },{ Landmark::kLeftEye1, "LEFT_EYE_1" },{ Landmark::kLeftEye2, "LEFT_EYE_2" },{ Landmark::kLeftEye3, "LEFT_EYE_3" },
      { Landmark::kLeftEye4, "LEFT_EYE_4" },{ Landmark::kLeftEye5, "LEFT_EYE_5" },
      { Landmark::kMouth0, "MOUTH_0" },{ Landmark::kMouth1, "MOUTH_1" },{ Landmark::kMouth2, "MOUTH_2" },{ Landmark::kMouth3, "MOUTH_3" },
      { Landmark::kMouth4, "MOUTH_4" },{ Landmark::kMouth5, "MOUTH_5" },{ Landmark::kMouth6, "MOUTH_6" },{ Landmark::kMouth7, "MOUTH_7" },
      { Landmark::kMouth8, "MOUTH_8" },{ Landmark::kMouth9, "MOUTH_9" },{ Landmark::kMouth10, "MOUTH_10" },{ Landmark::kMouth11, "MOUTH_11" },
      { Landmark::kMouth12, "MOUTH_12" },{ Landmark::kMouth13, "MOUTH_13" },{ Landmark::kMouth14, "MOUTH_14" },{ Landmark::kMouth15, "MOUTH_15" },
      { Landmark::kMouth16, "MOUTH_16" },{ Landmark::kMouth17, "MOUTH_17" }
    };

    mShapeClusters = {
      { BodyPart::kContour, "CONTOUR" },
      { BodyPart::kLeftEye, "LEFT_EYE" },
      { BodyPart::kLeftUpperEyelid, "LEFT_UPPER_EYELID" },
      { BodyPart::kLeftLowerEyelid, "LEFT_LOWER_EYELID" },
      { BodyPart::kRightEye, "RIGHT_EYE" },
      { BodyPart::kRightUpperEyelid, "RIGHT_UPPER_EYELID" },
      { BodyPart::kRightLowerEyelid, "RIGHT_LOWER_EYELID" },
      { BodyPart::kRightEyebrow, "RIGHT_EYE_BROW" },
      { BodyPart::kLeftEyebrow, "LEFT_EYE_BROW" },
      { BodyPart::kLeftEyebrow, "LEFT_EYE_BROW" },
      { BodyPart::kNose, "NOSE" },
      { BodyPart::kUpperLip, "UPPER_LIP" },
      { BodyPart::kLowerLip, "LOWER_LIP" },
      { BodyPart::kLeftLip, "LEFT_LIP" },
      { BodyPart::kRightLip, "RIGHT_LIP" },
      { BodyPart::kUndefined, "UNKNOWN_CLUSTER" }
    };

    mShape3D = {
      { -63.23, -10.97, 90.84 },
      { -61.55,   7.21, 89.43 },
      { -58.82,  25.30, 88.63 },
      { -54.83,  42.33, 83.25 },
      { -48.57,  56.84, 69.67 },
      { -38.40,  67.80, 55.43 },
      { -25.35,  75.10, 44.12 },
      { -10.90,  79.07, 32.87 },
      {   0.00,  78.67, 23.43 },
      {  10.90,  79.07, 32.87 },
      {  25.35,  75.10, 44.12 },
      {  38.40,  67.80, 55.43 },
      {  48.57,  56.84, 69.67 },
      {  54.83,  42.33, 83.25 },
      {  58.82,  25.30, 88.63 },
      {  61.55,   7.21, 89.43 },
      {  63.23, -10.97, 90.84 },
      { -50.36, -40.24, 35.36 },
      { -42.94, -47.48, 31.02 },
      { -33.69, -50.39, 27.59 },
      { -24.18, -50.00, 24.20 },
      { -14.97, -47.50, 21.05 },
      {  14.97, -47.50, 21.05 },
      {  24.18, -50.00, 24.20 },
      {  33.69, -50.39, 27.59 },
      {  42.94, -47.48, 31.02 },
      {  50.36, -40.24, 35.36 },
      {   0.00, -30.40, 22.32 },
      {   0.00, -20.21, 14.93 },
      {   0.00, -10.12,  7.19 },
      {   0.00,   0.00,  0.00 },	// Origin: kNose3
      {  -7.78,  14.80, 18.03 },
      {  -2.81,  16.32, 15.94 },
      {   0.00,  16.54, 14.12 },
      {   2.81,  16.32, 15.94 },
      {   7.78,  14.80, 18.03 },
      { -36.34, -24.73, 32.61 },
      { -30.84, -29.23, 32.09 },
      { -24.03, -29.83, 32.18 },
      { -18.18, -25.86, 32.70 },
      { -24.05, -23.21, 32.02 },
      { -30.41, -22.60, 32.08 },
      {  18.18, -25.86, 32.70 },
      {  24.03, -29.83, 32.18 },
      {  30.84, -29.23, 32.09 },
      {  36.34, -24.73, 32.61 },
      {  30.41, -22.60, 32.08 },
      {  24.05, -23.21, 32.02 },
      { -14.45,  39.27, 27.35 },
      {  -8.96,  34.16, 21.80 },
      {  -3.03,  30.09, 17.46 },
      {   0.00,  30.52, 13.32 },
      {   3.30,  30.09, 17.46 },
      {   8.96,  34.16, 21.80 },
      {  14.45,  39.27, 27.35 },
      {  13.35,  38.83, 25.24 },
      {   2.44,  39.57, 21.27 },
      {   0.00,  38.60, 18.20 },
      {  -2.44,  39.57, 21.27 },
      { -13.35,  38.83, 25.24 },
      {  -1.41,  37.25, 20.81 },
      {   0.00,  37.00, 16.39 },
      {   1.41,  37.25, 20.81 },
      {   1.41,  31.60, 22.62 },
      {   0.00,  29.53, 18.07 },
      {  -1.41,  31.60, 22.62 }
    };
  }

  const std::string& ShapeUtil::PointNameToString(Landmark iLandmark) const
  {
    auto it = mShapePoints.find(iLandmark);
    assert(it != mShapePoints.end());
    return it->second;
  }

  const std::string& ShapeUtil::ShapeClusterToString(BodyPart iShapeCluster) const
  {
    auto it = mShapeClusters.find(iShapeCluster);
    assert(it != mShapeClusters.end());
    return it->second;
  }

  BodyPart ShapeUtil::GetShapeClusterID(Landmark iLandmark) const
  {
    for (auto& it : mShapeParts)
      if (std::find(it.second.begin(), it.second.end(), iLandmark) != it.second.end())
        return it.first;

    return BodyPart::kUndefined;
  }

  const std::vector<Landmark>& ShapeUtil::GetLandmarks(BodyPart iClusterId) const
  {
    auto it = mShapeParts.find(iClusterId);
    assert(it != mShapeParts.end());
    return it->second;
  }
}
