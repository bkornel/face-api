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

      if (fw::get_value(iSettings, "poseFromShapeModel", value))
        mPoseFromShapeModel = fw::str::convert_to_boolean(value);
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

    // Where the rotation comes from. The shape model regresses one, and taking it is not a
    // shortcut but the more accurate of the two: recovering a rotation from the landmarks
    // means asking what rigid motion of the canonical model best projects onto them, and the
    // canonical model is not this face. On the sample clips - a face that crosses the frame
    // and never turns - that question answers with a yaw wandering over 15 degrees on one
    // and 31 on the other, while the regressed rotation stays inside 1.3. Nor is it a matter
    // of where the solver starts: seeded with the regressed rotation it converges to the
    // same place, because the minimum itself is in the wrong place.
    //
    // Only the translation is then solved for, which is a linear problem once the rotation
    // is known - see SolveTranslation - and has no second answer to be caught between.
    bool solved = false;

    if (mPoseFromShapeModel && iShape.hasRotation)
    {
      cv::Mat rotation(3, 3, CV_64FC1);

      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) rotation.at<double>(r, c) = iShape.rotation(r, c);

      if (SolveTranslation(iShape.rotation, sObjectPoints, imagePts, iCameraMatrix, oPose.tvec))
      {
        cv::Rodrigues(rotation, oPose.rvec);
        rotation.copyTo(oPose.extrinsics({ 0, 0, 3, 3 }));

        solved = true;
      }
    }

    if (!solved)
    {
      // Iterative rather than EPNP: on the 68-point set EPNP settles into a solution that
      // throws the jaw contour off by hundreds of pixels, while the iterative refinement
      // stays at the few pixels the landmarks themselves are worth.
      cv::solvePnP(sObjectPoints, imagePts, iCameraMatrix, sDistCoeffs, oPose.rvec, oPose.tvec,
                   false, cv::SOLVEPNP_ITERATIVE);
      cv::Rodrigues(oPose.rvec, oPose.extrinsics({ 0, 0, 3, 3 }));
    }

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

    // The fitted shape moved to where the head is. Transforming the canonical model instead,
    // as this used to, made every face's 3-D shape identical up to a rigid motion. Only the
    // placing is taken from the pose; the shape being placed is the face's own.
    oPose.shape3D = EstimateShape3D(oPose.extrinsics,
                                    iShape.shape3D.empty() ? sObjectPoints : iShape.shape3D);
    oPose.faceBox = mFaceBox;

    return true;
  }

  bool PoseEstimationDispatcher::SolveTranslation(const cv::Matx33d& iRotation, const ObjectPts& iObjectPts,
                                                  const ImagePts& iImagePts, const cv::Mat& iCameraMatrix,
                                                  cv::Mat& oTvec) const
  {
    const std::size_t count = (std::min)(iObjectPts.size(), iImagePts.size());
    if (count < 3U) return false;

    const double fx = iCameraMatrix.at<double>(0, 0);
    const double fy = iCameraMatrix.at<double>(1, 1);
    const double cx = iCameraMatrix.at<double>(0, 2);
    const double cy = iCameraMatrix.at<double>(1, 2);

    // A landmark at X projects to ((f (RX + t)) / (RX + t)_z) + c. Multiplying out the
    // perspective divide leaves that linear in t, two equations per landmark:
    //
    //   fx tx - (u - cx) tz = (u - cx) (RX)_z - fx (RX)_x
    //   fy ty - (v - cy) tz = (v - cy) (RX)_z - fy (RX)_y
    //
    // Sixty-eight landmarks give a hundred and thirty-six of them for three unknowns, and
    // the least-squares solution needs no starting guess and has no second answer.
    cv::Mat a(static_cast<int>(count) * 2, 3, CV_64FC1, cv::Scalar(0.0));
    cv::Mat b(static_cast<int>(count) * 2, 1, CV_64FC1);

    for (std::size_t i = 0U; i < count; ++i)
    {
      const cv::Point3d& objectPt = iObjectPts[i];
      const cv::Vec3d turned = iRotation * cv::Vec3d(objectPt.x, objectPt.y, objectPt.z);

      const double u = iImagePts[i].x - cx;
      const double v = iImagePts[i].y - cy;

      const int row = static_cast<int>(i) * 2;

      a.at<double>(row, 0) = fx;
      a.at<double>(row, 2) = -u;
      b.at<double>(row, 0) = u * turned[2] - fx * turned[0];

      a.at<double>(row + 1, 1) = fy;
      a.at<double>(row + 1, 2) = -v;
      b.at<double>(row + 1, 0) = v * turned[2] - fy * turned[1];
    }

    cv::Mat tvec;
    if (!cv::solve(a, b, tvec, cv::DECOMP_SVD)) return false;

    // Behind the camera is not a pose, it is a sign error somewhere upstream
    if (tvec.at<double>(2, 0) <= 0.0) return false;

    oTvec = tvec;

    return true;
  }

  PoseEstimationDispatcher::ObjectPts PoseEstimationDispatcher::EstimateShape3D(const cv::Mat& iExtrinsics,
                                                                                const ObjectPts& iShape) const
  {
    ObjectPts shape3D;
    shape3D.reserve(iShape.size());

    for (const auto& objPt : iShape)
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
