#include "Model/FaceModel.h"

namespace face
{
  FaceModel& FaceModel::GetInstance()
  {
    static FaceModel sInstance;
    return sInstance;
  }

  FaceModel::FaceModel()
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
      { Landmark::kContour0, "CONTOUR_0" }, { Landmark::kContour1, "CONTOUR_1" }, { Landmark::kContour2, "CONTOUR_2" }, { Landmark::kContour3, "CONTOUR_3" }, { Landmark::kContour4, "CONTOUR_4" }, { Landmark::kContour5, "CONTOUR_5" }, { Landmark::kContour6, "CONTOUR_6" }, { Landmark::kContour7, "CONTOUR_7" }, { Landmark::kContour8, "CONTOUR_8" }, { Landmark::kContour9, "CONTOUR_9" }, { Landmark::kContour10, "CONTOUR_10" }, { Landmark::kContour11, "CONTOUR_11" }, { Landmark::kContour12, "CONTOUR_12" }, { Landmark::kContour13, "CONTOUR_13" }, { Landmark::kContour14, "CONTOUR_14" }, { Landmark::kContour15, "CONTOUR_15" }, { Landmark::kContour16, "CONTOUR_16" }, { Landmark::kRightEyebrow0, "RIGHT_EYE_BROW_0" }, { Landmark::kRightEyebrow1, "RIGHT_EYE_BROW_1" }, { Landmark::kRightEyebrow2, "RIGHT_EYE_BROW_2" }, { Landmark::kRightEyebrow3, "RIGHT_EYE_BROW_3" }, { Landmark::kRightEyebrow4, "RIGHT_EYE_BROW_4" }, { Landmark::kLeftEyebrow0, "LEFT_EYE_BROW_0" }, { Landmark::kLeftEyebrow1, "LEFT_EYE_BROW_1" }, { Landmark::kLeftEyebrow2, "LEFT_EYE_BROW_2" }, { Landmark::kLeftEyebrow3, "LEFT_EYE_BROW_3" }, { Landmark::kLeftEyebrow4, "LEFT_EYE_BROW_4" }, { Landmark::kNose0, "NOSE_0" }, { Landmark::kNose1, "NOSE_1" }, { Landmark::kNose2, "NOSE_2" }, { Landmark::kNose3, "NOSE_3" }, { Landmark::kNose4, "NOSE_4" }, { Landmark::kNose5, "NOSE_5" }, { Landmark::kNose6, "NOSE_6" }, { Landmark::kNose7, "NOSE_7" }, { Landmark::kNose8, "NOSE_8" }, { Landmark::kRightEye0, "RIGHT_EYE_0" }, { Landmark::kRightEye1, "RIGHT_EYE_1" }, { Landmark::kRightEye2, "RIGHT_EYE_2" }, { Landmark::kRightEye3, "RIGHT_EYE_3" }, { Landmark::kRightEye4, "RIGHT_EYE_4" }, { Landmark::kRightEye5, "RIGHT_EYE_5" }, { Landmark::kLeftEye0, "LEFT_EYE_0" }, { Landmark::kLeftEye1, "LEFT_EYE_1" }, { Landmark::kLeftEye2, "LEFT_EYE_2" }, { Landmark::kLeftEye3, "LEFT_EYE_3" }, { Landmark::kLeftEye4, "LEFT_EYE_4" }, { Landmark::kLeftEye5, "LEFT_EYE_5" }, { Landmark::kMouth0, "MOUTH_0" }, { Landmark::kMouth1, "MOUTH_1" }, { Landmark::kMouth2, "MOUTH_2" }, { Landmark::kMouth3, "MOUTH_3" }, { Landmark::kMouth4, "MOUTH_4" }, { Landmark::kMouth5, "MOUTH_5" }, { Landmark::kMouth6, "MOUTH_6" }, { Landmark::kMouth7, "MOUTH_7" }, { Landmark::kMouth8, "MOUTH_8" }, { Landmark::kMouth9, "MOUTH_9" }, { Landmark::kMouth10, "MOUTH_10" }, { Landmark::kMouth11, "MOUTH_11" }, { Landmark::kMouth12, "MOUTH_12" }, { Landmark::kMouth13, "MOUTH_13" }, { Landmark::kMouth14, "MOUTH_14" }, { Landmark::kMouth15, "MOUTH_15" }, { Landmark::kMouth16, "MOUTH_16" }, { Landmark::kMouth17, "MOUTH_17" }, { Landmark::kMouth18, "MOUTH_18" }, { Landmark::kMouth19, "MOUTH_19" }
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
      {  -65.05,  -23.49,  101.09 },  // Contour
      {  -63.32,   -5.99,   98.95 },
      {  -59.89,    9.96,   97.38 },
      {  -56.48,   24.35,   94.00 },
      {  -51.65,   39.82,   85.86 },
      {  -42.98,   52.22,   71.50 },
      {  -32.65,   59.65,   53.99 },
      {  -19.47,   65.86,   37.86 },
      {    0.01,   69.38,   31.57 },
      {   19.50,   65.63,   37.92 },
      {   32.60,   59.68,   54.03 },
      {   42.85,   52.43,   71.44 },
      {   51.44,   40.20,   85.80 },
      {   56.28,   24.80,   93.92 },
      {   59.55,   10.29,   97.41 },
      {   62.69,   -5.75,   99.07 },
      {   64.35,  -23.44,  100.96 },
      {  -50.82,  -42.76,   45.57 },  // Eyebrows
      {  -43.49,  -48.42,   34.97 },
      {  -34.13,  -50.28,   27.58 },
      {  -25.14,  -49.48,   23.06 },
      {  -17.03,  -47.23,   20.91 },
      {   16.28,  -47.43,   20.96 },
      {   24.44,  -49.74,   23.19 },
      {   33.48,  -50.59,   27.79 },
      {   43.00,  -48.70,   35.11 },
      {   50.33,  -42.90,   45.70 },
      {   -0.13,  -30.40,   18.17 },  // Nose
      {   -0.09,  -19.73,   10.22 },
      {    0.00,   -9.11,    1.96 },
      {    0.00,    0.00,    0.00 },  // Origin: the nose tip
      {  -10.76,    7.94,   19.60 },
      {   -6.26,    8.86,   15.60 },
      {   -0.15,   10.06,   13.74 },
      {    5.91,    8.84,   15.63 },
      {   10.36,    7.88,   19.64 },
      {  -38.44,  -30.05,   39.56 },  // Eyes
      {  -32.80,  -33.45,   32.34 },
      {  -24.77,  -33.56,   32.18 },
      {  -17.29,  -29.75,   34.38 },
      {  -24.03,  -27.89,   32.44 },
      {  -32.42,  -27.47,   34.45 },
      {   16.36,  -29.76,   34.67 },
      {   23.88,  -33.65,   32.54 },
      {   32.11,  -33.44,   32.55 },
      {   37.93,  -29.94,   39.62 },
      {   31.77,  -27.59,   34.52 },
      {   23.28,  -27.88,   32.61 },
      {  -22.72,   29.42,   28.98 },  // Outer lip
      {  -14.73,   24.16,   19.04 },
      {   -5.21,   20.71,   13.69 },
      {   -0.13,   21.70,   13.15 },
      {    4.92,   20.71,   13.71 },
      {   14.40,   24.16,   19.12 },
      {   21.88,   29.46,   29.17 },
      {   13.93,   33.13,   20.70 },
      {    7.11,   35.69,   16.89 },
      {   -0.11,   36.14,   16.00 },
      {   -7.28,   35.66,   16.75 },
      {  -14.01,   33.15,   20.55 },
      {  -20.48,   28.97,   28.32 },  // Inner lip
      {   -6.70,   26.65,   18.20 },
      {   -0.23,   26.49,   16.62 },
      {    6.28,   26.70,   18.22 },
      {   20.41,   29.04,   28.65 },
      {    6.12,   28.78,   18.03 },
      {   -0.25,   29.10,   17.25 },
      {   -6.54,   28.72,   18.10 }
    };

    mFrontalShape2D.reserve(mShape3D.size());
    for (const auto& pt : mShape3D)
      mFrontalShape2D.emplace_back(pt.x, pt.y);

    // The anatomy of the 68-point layout: the contour and brow chains, the nose, and the
    // closed rings of the eyes and both lips.
    mConnections = {
      // Contour
      { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 }, { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 8 },
      { 8, 9 }, { 9, 10 }, { 10, 11 }, { 11, 12 }, { 12, 13 }, { 13, 14 }, { 14, 15 }, { 15, 16 },
      // Eyebrows
      { 17, 18 }, { 18, 19 }, { 19, 20 }, { 20, 21 },
      { 22, 23 }, { 23, 24 }, { 24, 25 }, { 25, 26 },
      // Nose
      { 27, 28 }, { 28, 29 }, { 29, 30 },
      { 31, 32 }, { 32, 33 }, { 33, 34 }, { 34, 35 },
      // Eyes
      { 36, 37 }, { 37, 38 }, { 38, 39 }, { 39, 40 }, { 40, 41 }, { 41, 36 },
      { 42, 43 }, { 43, 44 }, { 44, 45 }, { 45, 46 }, { 46, 47 }, { 47, 42 },
      // Outer and inner lips
      { 48, 49 }, { 49, 50 }, { 50, 51 }, { 51, 52 }, { 52, 53 }, { 53, 54 },
      { 54, 55 }, { 55, 56 }, { 56, 57 }, { 57, 58 }, { 58, 59 }, { 59, 48 },
      { 60, 61 }, { 61, 62 }, { 62, 63 }, { 63, 64 },
      { 64, 65 }, { 65, 66 }, { 66, 67 }, { 67, 60 }
    };
  }

  const std::string& FaceModel::PointNameToString(Landmark iLandmark) const
  {
    auto it = mShapePoints.find(iLandmark);
    CV_DbgAssert(it != mShapePoints.end());
    return it->second;
  }

  const std::string& FaceModel::ShapeClusterToString(BodyPart iShapeCluster) const
  {
    auto it = mShapeClusters.find(iShapeCluster);
    CV_DbgAssert(it != mShapeClusters.end());
    return it->second;
  }

  BodyPart FaceModel::GetShapeClusterID(Landmark iLandmark) const
  {
    for (auto& it : mShapeParts)
      if (std::find(it.second.begin(), it.second.end(), iLandmark) != it.second.end())
        return it.first;

    return BodyPart::kUndefined;
  }

  const std::vector<Landmark>& FaceModel::GetLandmarks(BodyPart iClusterId) const
  {
    auto it = mShapeParts.find(iClusterId);
    CV_DbgAssert(it != mShapeParts.end());
    return it->second;
  }
}
