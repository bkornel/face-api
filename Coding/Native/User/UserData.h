#pragma once

#include <memory>

#include "Common/ShapeUtil.h"
#include "Framework/UtilOCV.h"

namespace face
{
	class UserData
	{
	public:
		typedef std::shared_ptr<UserData> Shared;

		UserData() = default;

		UserData(const UserData& iOther) = default;

		virtual ~UserData() = default;

		UserData& operator=(const UserData& iOther) = default;

		// Get: General & detection
		inline const cv::Rect& GetFaceRect() const { return mFaceRect; }
		inline const cv::Mat& GetFaceTemplate() const { return mFaceTemplate; }
		inline const cv::Vec2i& GetFaceRectOffset() const { return mFaceRectOffset; }
		inline const fw::ocv::VectorPt3D& GetFaceBox() const { return mFaceBox; }

		// Get: Shape
		inline const fw::ocv::VectorPt2D& GetShape2D() const { return mShape2D; }
		inline const fw::ocv::VectorPt3D& GetShape3D() const { return mShape3D; }
		inline const fw::ocv::VectorPt2D& GetNormShape2D() const { return mNormShape2D; }
		inline const fw::ocv::VectorPt3D& GetNormShape3D() const { return mNormShape3D; }
		inline const cv::Point2d& GetPoint2d(Landmark iIdx) const
		{
			const int idx = static_cast<int>(iIdx);
			assert(idx >= 0 && idx < mShape2D.size());
			return mShape2D[idx];
		}
		inline const cv::Point3d& GetPoint3d(Landmark iIdx) const
		{
			const int idx = static_cast<int>(iIdx);
			assert(idx >= 0 && idx < mShape3D.size());
			return mShape3D[idx];
		}

		// Get: Pose
		inline const cv::Vec3d& GetRPY() const { return mRPY; }
		inline const cv::Vec3d& GetPosition3D() const { return mPosition3D; }
		inline const cv::Mat& GetCameraMatrix() const { return mCameraMatrix; }
		inline const cv::Mat& GetExtrinsics() const { return mExtrinsics; }
		inline const cv::Mat& GetRvec() const { return mRvec; }
		inline const cv::Mat& GetTvec() const { return mTvec; }

		// Set: General & detection
		inline void SetFaceRect(const cv::Rect& iFaceRect) 
		{ 
			mFaceRectOffset = (iFaceRect.tl() - mFaceRect.tl());
			mFaceRect = iFaceRect; 
		}
		inline void SetFaceTemplate(const cv::Mat& iFaceTemplate) { mFaceTemplate = iFaceTemplate.clone(); }
		inline void SetFaceBox(const fw::ocv::VectorPt3D& iFaceBox) { mFaceBox = iFaceBox; }

		// Set: Shape
		inline void SetShape3D(const fw::ocv::VectorPt3D& iShape3D) { mShape3D = iShape3D; }
		inline void SetShape2D(const fw::ocv::VectorPt2D& iShape2D) { mShape2D = iShape2D; }
		inline void SetNormShapes(const fw::ocv::VectorPt2D& iNormShape2D, const fw::ocv::VectorPt3D& iNormShape3D)
		{
			mNormShape2D = iNormShape2D;
			mNormShape3D = iNormShape3D;
		}

		// Set: Pose
		inline void SetPose(const cv::Vec3d& iRPY, const cv::Vec3d& iPosition3D) 
		{ 
			mRPY = iRPY;
			mPosition3D = iPosition3D;
		}
		inline void SetCameraMatrix(const cv::Mat& iCameraMatrix) { mCameraMatrix = iCameraMatrix.clone(); }
		inline void SetExtrinsics(const cv::Mat& iExtrinsics, const cv::Mat& iRvec, const cv::Mat& iTvec) 
		{
			mExtrinsics = iExtrinsics.clone(); 
			mRvec = iRvec.clone();
			mTvec = iTvec.clone();
		}

	private:
		// Face
		cv::Rect mFaceRect;
		cv::Mat mFaceTemplate;
		cv::Vec2i mFaceRectOffset;
		fw::ocv::VectorPt3D mFaceBox;

		// Pose
		cv::Vec3d mRPY;
		cv::Vec3d mPosition3D;
		cv::Mat mCameraMatrix;
		cv::Mat mExtrinsics;
		cv::Mat mRvec;
		cv::Mat mTvec;

		// Shape
		fw::ocv::VectorPt2D mShape2D;
		fw::ocv::VectorPt2D mNormShape2D;
		fw::ocv::VectorPt3D mShape3D;
		fw::ocv::VectorPt3D mNormShape3D;
	};
}
