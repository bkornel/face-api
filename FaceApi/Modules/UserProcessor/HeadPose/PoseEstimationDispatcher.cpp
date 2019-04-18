#define _USE_MATH_DEFINES

#include "Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.h"

#include "Common/Configuration.h"
#include "Framework/UtilString.h"
#include "User/User.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  const PoseEstimationDispatcher::ObjectPts PoseEstimationDispatcher::sObjectPoints =
    ShapeUtil::GetInstance().GetShape3D();

  fw::ErrorCode PoseEstimationDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::ocv::get_value(iSettings, "faceBoxOffset", value))
        mFaceBoxOffset = fw::str::convert_to_number<double>(value);

      if (fw::ocv::get_value(iSettings, "estimateReprojection", value))
        mEstimateReprojection = fw::str::convert_to_boolean(value);
    }

    return fw::ErrorCode::OK;
  }

  bool PoseEstimationDispatcher::Dispatch(User& ioUser)
  {
    // 1. Estimate the head pose
    estimatePose(ioUser.GetShape2D());

    // 2. Estimate the rotated 3-D shape points
    estimateShape3D();

    // 3. Estimate the bounding box of the rotated face
    estimateFaceBox();

    // 3. Set all user data
    ioUser.SetPose(mRPY, mPosition);
    ioUser.SetCameraMatrix(mCameraMatrix);
    ioUser.SetExtrinsics(mExtrinsics, mRvec, mTvec);
    ioUser.SetShape3D(mShape3D);
    ioUser.SetFaceBox(mFaceBox);

    return true;
  }

  void PoseEstimationDispatcher::estimatePose(const ImagePts& iImagePts)
  {
    static const cv::Mat sDistCoeffs = cv::Mat::zeros(4, 1, CV_64FC1);

    mExtrinsics = cv::Mat::eye(4, 4, CV_64FC1);

    // Solve for pose
    cv::solvePnP(sObjectPoints, iImagePts, mCameraMatrix, sDistCoeffs, mRvec, mTvec, false, cv::SOLVEPNP_EPNP);
    cv::Rodrigues(mRvec, mExtrinsics({ 0, 0, 3, 3 }));

    if (mEstimateReprojection)
    {
      // Testing:
      ImagePts imagePointsRP;
      cv::projectPoints(sObjectPoints, mRvec, mTvec, mCameraMatrix, sDistCoeffs, imagePointsRP);

      double totalErr = 0.0;
      for (size_t i = 0; i < iImagePts.size(); i++)
      {
        double err = cv::norm(cv::Mat(iImagePts[i]), cv::Mat(imagePointsRP[i]), cv::NORM_L2);
        totalErr += err * err;
      }

      totalErr = std::sqrt(totalErr / iImagePts.size());
      LOG(DEBUG) << "Re-projection error: " << totalErr << " px.";
    }

    for (int i = 0; i < 3; ++i)
      mPosition[i] = mExtrinsics.at<double>(i, 3) = mTvec.at<double>(i, 0);

    // Get roll-pitch-yaw
    cv::Mat cameraMatrix, rotation, translation;
    cv::decomposeProjectionMatrix(mExtrinsics({ 0, 0, 4, 3 }), cameraMatrix, rotation, translation,
      cv::noArray(), cv::noArray(), cv::noArray(), mRPY);

    mRPY = { FW_DEG_TO_RAD(mRPY[2]), FW_DEG_TO_RAD(mRPY[0]), FW_DEG_TO_RAD(mRPY[1]) };
  }

  void PoseEstimationDispatcher::estimateShape3D()
  {
    mShape3D.clear();
    mShape3D.reserve(sObjectPoints.size());

    for (const auto& objPt : sObjectPoints)
    {
      const cv::Mat& objPtRot = mExtrinsics * cv::Mat_<double>({ 4, 1 }, { objPt.x, objPt.y, objPt.z, 1.0 });

      mShape3D.push_back({
        objPtRot.at<double>(0, 0),
        objPtRot.at<double>(1, 0),
        objPtRot.at<double>(2, 0)
        });
    }
  }

  void PoseEstimationDispatcher::estimateFaceBox()
  {
    assert(!mShape3D.empty());

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

    // See the order in PoseUtil.h
    mFaceBox =
    {
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
