#pragma once

#include "Common/Configuration.h"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

#include "User/User.h"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"
#include "Messages/ActiveUsersMessage.h"

namespace face
{
	class UserManager :
		public fw::Module,
		public fw::Port<ActiveUsersMessage::Shared>
	{
	public:
		UserManager();

		virtual ~UserManager() = default;

		ActiveUsersMessage::Shared Process(ImageMessage::Shared iImage, RoiMessage::Shared iDetections);

		void Clear() override;

		std::size_t GetActiveUserSize() const;

		inline bool IsAllPossibleUsersTracked() const
		{
			return GetMaxUsers() == GetActiveUserSize();
		}

		inline int GetMaxUsers() const
		{
			return mMaxUsers;
		}

		inline void SetMinFaceSize(const cv::Size& iMinFaceSize)
		{
			mMinFaceSize = iMinFaceSize;
		}

	private:
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		void PreprocessUsers();

		void ProcessDetections(RoiMessage::Shared iDetections);

		void TrackUsers(ImageMessage::Shared iImage);

		void PostprocessUsers();

		void MergeDetectionsAndUsers(std::vector<cv::Rect>& ioFaceROIs);

		bool MatchTemplate(ImageMessage::Shared iImage, User::Shared ioUser, cv::Rect& oFaceRect);

		void RemoveInactiveUsers(bool forceToDelete = false);

		std::vector<User::Shared> mUsers;   ///< The vector storing all users
		fw::Stopwatch mRemoveSW;
		long long mTimestamp = 0;

		cv::Size mMinFaceSize;
		int mMaxUsers = 1;
		float mUserOverlap = 0.2F;
		float mUserAwaySec = 15.0F;
		float mTemplateScale = 1.0f;
		float mTemplateScaleInv = 1.0f;
	};
}