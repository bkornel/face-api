#pragma once

#include "User/UserDispatcher.hpp"
#include "Modules/UserProcessor/ShapeModel/ShapeModel.h"

namespace face
{
	class User;
	class ShapeModel;
	class UserData;

	class ShapeModelDispatcher :
		public UserDispatcher
	{
		typedef std::map<int, ShapeModel::Shared> ShapeModels;

	public:
		ShapeModelDispatcher() = default;

		virtual ~ShapeModelDispatcher();

		fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

		bool Dispatch(User& ioUser) override;

		inline ShapeModel::Shared GetShapeModel() const
		{
			return mShapeModel;
		}

		inline void SetFrame(const cv::Mat& iFrame)
		{
			mFrame = iFrame;
		}

	private:
		static ShapeModels sShapeModels;

		bool Fit(User& ioUser);

		bool UpdateTemplate(User& ioUser);

		ShapeModel::Shared mShapeModel = nullptr;
		std::vector<int> mUpdatedUserIDs;

		cv::Mat mFrame;
		std::vector<int> mWinDetection = { 11, 9, 7 };
		std::vector<int> mWinTracking = { 7 };
		int mNoIter = 10;
		double mClamp = 3.0;
		double mFTol = 0.01;
		bool mFailureCheck = false;
	};
}
