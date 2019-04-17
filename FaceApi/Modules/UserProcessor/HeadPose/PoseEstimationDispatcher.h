#pragma once

#include "Framework/UtilOCV.h"
#include "User/UserDispatcher.hpp"

namespace face
{
	class User;

	class PoseEstimationDispatcher :
		public UserDispatcher
	{
		using ImagePts = fw::ocv::VectorPt2D;
		using ObjectPts = fw::ocv::VectorPt3D;

	public:
		PoseEstimationDispatcher() = default;

		virtual ~PoseEstimationDispatcher() = default;

		fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

		bool Dispatch(User& ioUser) override;

		inline void EstimateCameraMatrix(const cv::Size& iSize)
		{
			mCameraMatrix = fw::ocv::get_camera_matrix(iSize);
		}

	private:
		static const ObjectPts sObjectPoints;

		void estimatePose(const ImagePts& iImagePts);

		void estimateShape3D();

		void estimateFaceBox();

		cv::Mat mCameraMatrix;
		cv::Mat mExtrinsics;
		cv::Mat mRvec;
		cv::Mat mTvec;
		cv::Vec3d mRPY;
		cv::Vec3d mPosition;
		ObjectPts mShape3D;
		ObjectPts mFaceBox;

		bool mEstimateReprojection = false;
		double mFaceBoxOffset = 5.0;
	};
}
