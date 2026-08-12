#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/MathExtensions.h"
#include "Modules/HeadPose/PoseEstimationDispatcher.h"

#include "Model/FaceModel.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/calib3d.hpp>

namespace face
{
  const PoseEstimationDispatcher::ObjectPts PoseEstimationDispatcher::sObjectPoints =
    FaceModel::GetInstance().GetShape3D();

  fw::ErrorCode PoseEstimationDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "faceBoxOffset", value))
        mFaceBoxOffset = fw::str::convert_to_number<double>(value);

      if (fw::get_value(iSettings, "estimateReprojection", value))
        mEstimateReprojection = fw::str::convert_to_boolean(value);
    }

    mFaceBox = CreateFaceBox();

    return fw::ErrorCode::OK;
  }

  bool PoseEstimationDispatcher::Estimate(const ShapeDescriptor& iShape, const cv::Mat& iCameraMatrix, PoseDescriptor& oPose) const
  {
    static const cv::Mat sDistCoeffs = cv::Mat::zeros(4, 1, CV_64FC1);

    const ImagePts& imagePts = iShape.shape2D;

    oPose.trackId = iShape.trackId;
    oPose.cameraMatrix = iCameraMatrix;
    oPose.extrinsics = cv::Mat::eye(4, 4, CV_64FC1);

    // Solve for pose
    cv::solvePnP(sObjectPoints, imagePts, iCameraMatrix, sDistCoeffs, oPose.rvec, oPose.tvec, false, cv::SOLVEPNP_EPNP);
    cv::Rodrigues(oPose.rvec, oPose.extrinsics({ 0, 0, 3, 3 }));

    if (mEstimateReprojection)
    {
      ImagePts imagePointsRP;
      cv::projectPoints(sObjectPoints, oPose.rvec, oPose.tvec, iCameraMatrix, sDistCoeffs, imagePointsRP);

      double totalErr = 0.0;
      for (size_t i = 0; i < imagePts.size(); i++)
      {
        double err = cv::norm(cv::Mat(imagePts[i]), cv::Mat(imagePointsRP[i]), cv::NORM_L2);
        totalErr += err * err;
      }

      totalErr = std::sqrt(totalErr / imagePts.size());
      LOG(DEBUG) << "Re-projection error: " << totalErr << " px.";
    }

    for (int i = 0; i < 3; ++i)
      oPose.position3D[i] = oPose.extrinsics.at<double>(i, 3) = oPose.tvec.at<double>(i, 0);

    // Get roll-pitch-yaw
    cv::Mat cameraMatrix, rotation, translation;
    cv::decomposeProjectionMatrix(oPose.extrinsics({ 0, 0, 4, 3 }), cameraMatrix, rotation, translation, cv::noArray(), cv::noArray(), cv::noArray(), oPose.rpy);

    oPose.rpy = { fw::deg_to_rad(oPose.rpy[2]), fw::deg_to_rad(oPose.rpy[0]), fw::deg_to_rad(oPose.rpy[1]) };

    oPose.shape3D = EstimateShape3D(oPose.extrinsics);
    oPose.faceBox = mFaceBox;

    return true;
  }

  PoseEstimationDispatcher::ObjectPts PoseEstimationDispatcher::EstimateShape3D(const cv::Mat& iExtrinsics) const
  {
    ObjectPts shape3D;
    shape3D.reserve(sObjectPoints.size());

    for (const auto& objPt : sObjectPoints)
    {
      const cv::Mat& objPtRot = iExtrinsics * cv::Mat_<double>({ 4, 1 }, { objPt.x, objPt.y, objPt.z, 1.0 });

      shape3D.emplace_back(
        objPtRot.at<double>(0, 0),
        objPtRot.at<double>(1, 0),
        objPtRot.at<double>(2, 0)
      );
    }

    return shape3D;
  }

  PoseEstimationDispatcher::ObjectPts PoseEstimationDispatcher::CreateFaceBox() const
  {
    CV_DbgAssert(!sObjectPoints.empty());

    cv::Point3d minPt = sObjectPoints[0];
    cv::Point3d maxPt = sObjectPoints[0];

    for (const auto& pt : sObjectPoints)
    {
      minPt.x = (std::min)(minPt.x, pt.x);
      minPt.y = (std::min)(minPt.y, pt.y);
      minPt.z = (std::min)(minPt.z, pt.z);

      maxPt.x = (std::max)(maxPt.x, pt.x);
      maxPt.y = (std::max)(maxPt.y, pt.y);
      maxPt.z = (std::max)(maxPt.z, pt.z);
    }

    // See the order in PoseGeometry.h
    return {
      // Front face
      { minPt.x - mFaceBoxOffset, minPt.y - mFaceBoxOffset, minPt.z },
      { maxPt.x + mFaceBoxOffset, minPt.y - mFaceBoxOffset, minPt.z },
      { minPt.x - mFaceBoxOffset, maxPt.y + mFaceBoxOffset, minPt.z },
      { maxPt.x + mFaceBoxOffset, maxPt.y + mFaceBoxOffset, minPt.z },

      // Rear face
      { minPt.x + mFaceBoxOffset, minPt.y + mFaceBoxOffset, maxPt.z },
      { maxPt.x - mFaceBoxOffset, minPt.y + mFaceBoxOffset, maxPt.z },
      { minPt.x + mFaceBoxOffset, maxPt.y - mFaceBoxOffset, maxPt.z },
      { maxPt.x - mFaceBoxOffset, maxPt.y - mFaceBoxOffset, maxPt.z }
    };
  }
}
