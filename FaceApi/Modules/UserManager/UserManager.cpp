#include <iomanip>
#include <easyloggingpp/easyloggingpp.h>

#include "Framework/Profiler.h"

#include "Modules/UserManager/UserManager.h"

namespace face
{
	UserManager::UserManager() :
		fw::Module("UserManager")
	{
	}

	fw::ErrorCode UserManager::InitializeInternal(const cv::FileNode& iSettings)
	{
		if (!iSettings.empty())
		{
			std::string value;

			if (fw::ocv::get_value(iSettings, "maxUsers", value))
				mMaxUsers = fw::str::convert_to_number<int>(value);

			if (fw::ocv::get_value(iSettings, "userOverlap", value))
				mUserOverlap = fw::str::convert_to_number<float>(value);

			if (fw::ocv::get_value(iSettings, "userAwaySec", value))
				mUserAwaySec = fw::str::convert_to_number<float>(value);

			if (fw::ocv::get_value(iSettings, "templateScale", value))
			{
				mTemplateScale = fw::str::convert_to_number<float>(value);
				mTemplateScale = std::max(std::min(mTemplateScale, 1.0F), 0.2F);
				mTemplateScaleInv = (1.0F / mTemplateScale);
			}
		}

		mRemoveSW.Start();

		return fw::ErrorCode::OK;
	}

	void UserManager::Clear()
	{
		for (const auto& user : mUsers)
			user->SetStatus(User::Status::Inactive);

		RemoveInactiveUsers(true);
	}

	ActiveUsersMessage::Shared UserManager::Process(ImageMessage::Shared iImage, RoiMessage::Shared iDetections)
	{
		if (!iImage || iImage->IsEmpty()) return nullptr;

		FACE_PROFILER(User_Manager);

		mTimestamp = iImage->GetTimestamp();

		// Active users to inactive and set is-detected to false
		PreprocessUsers();

		// Merge users and detections and create new users
		ProcessDetections(iDetections);

		// Track users by their faces
		TrackUsers(iImage);

		// Remove old user history entries and inactive users
		PostprocessUsers();

		return GetActiveUserSize() > 0 ? std::make_shared<ActiveUsersMessage>(mUsers, iImage->GetFrameId(), iImage->GetTimestamp()) : nullptr;
	}

	void UserManager::PreprocessUsers()
	{
		for (const auto& user : mUsers)
		{
			const bool inactivate =
				// Must be active
				user->IsActive() &&
				// Minimal resolution
				((user->GetFaceRect().width < mMinFaceSize.width) ||
				(user->GetFaceRect().height < mMinFaceSize.height) ||
					// Detected a long time ago
					(mTimestamp - user->GetLastDetectionTs()) > mUserAwaySec * 1000.0F);

			if (inactivate)
				user->SetStatus(User::Status::Inactive);

			// Set the status to-be-tracked after detection and before tracking
			if (user->IsDetected())
				user->SetStatus(User::Status::ToBeTracked);
		}
	}

	void UserManager::ProcessDetections(RoiMessage::Shared iDetections)
	{
		if (!iDetections || iDetections->IsEmpty()) return;

		static int sLastUserID = 0;

		std::vector<cv::Rect> faceROIs = iDetections->GetROIs();

		// Checking the overlap between detector's rectangles and users
		MergeDetectionsAndUsers(faceROIs);

		// Add new users
		for (const auto& r : faceROIs)
		{
			if (GetActiveUserSize() >= mMaxUsers) break;

			mUsers.push_back(std::make_shared<User>(r, sLastUserID, mTimestamp));
			LOG(INFO) << "New user has been recognized, Welcome User(" << sLastUserID << ")!";
			sLastUserID++;
		}
	}

	void UserManager::TrackUsers(ImageMessage::Shared iImage)
	{
		FACE_PROFILER(Track_Users);

		for (const auto& user : mUsers)
		{
			if (!user->IsActive())
				continue;

			if (!user->IsDetected())
			{
				cv::Rect newFaceRect;
				if (!MatchTemplate(iImage, user, newFaceRect))
				{
					user->SetStatus(User::Status::Inactive);
					continue;
				}

				user->SetFaceRect(newFaceRect);
			}

			const cv::Mat& frameGray = iImage->GetFrameGray();
			user->SetFaceTemplate(frameGray(user->GetFaceRect()));
			user->SetLastUpdateTs(iImage->GetTimestamp());
		}
	}

	void UserManager::PostprocessUsers()
	{
		if (mRemoveSW.GetElapsedTimeSec(false) > mUserAwaySec)
		{
			RemoveInactiveUsers();
			mRemoveSW.Reset();
		}
	}

	void UserManager::MergeDetectionsAndUsers(std::vector<cv::Rect>& ioFaceROIs)
	{
		for (auto& user : mUsers)
		{
			const cv::Rect& userFR = user->GetFaceRect();

			for (auto fr = ioFaceROIs.begin(); fr != ioFaceROIs.end();)
			{
				if (fw::ocv::overlap_ratio(userFR, *fr) > mUserOverlap)
				{
					// An active user is detected
					if (user->IsActive())
					{
						// This also sets the status to detected
						user->SetDetectionData(*fr, mTimestamp);
					}
					// An inactive user is detected
					else
					{
						if (GetActiveUserSize() >= mMaxUsers) continue;

						// This also sets the status to detected
						user->SetDetectionData(*fr, mTimestamp);
					}

					fr = ioFaceROIs.erase(fr);
				}
				else
				{
					fr++;
				}
			}
		}
	}

	bool UserManager::MatchTemplate(ImageMessage::Shared iImage, User::Shared ioUser, cv::Rect& oFaceRect)
	{
		assert(mTemplateScale > 0.0F && mTemplateScale <= 1.0F);

		oFaceRect = {};

		const cv::Mat& frame = iImage->GetResizedGray(mTemplateScale);
		cv::Mat faceTpl = ioUser->GetFaceTemplate();

		if (std::abs(mTemplateScale - 1.0F) > std::numeric_limits<float>::epsilon())
			cv::resize(faceTpl, faceTpl, {}, mTemplateScale, mTemplateScale);

		if ((faceTpl.cols > frame.cols) || (faceTpl.rows > frame.rows))
			return false;

		cv::Mat result(frame.cols - faceTpl.cols + 1, frame.rows - faceTpl.rows + 1, CV_32FC1);
		cv::matchTemplate(frame, faceTpl, result, cv::TM_CCOEFF_NORMED);

		double maxVal = 0.0;
		cv::Point maxLoc;
		cv::minMaxLoc(result, nullptr, &maxVal, nullptr, &maxLoc);

		oFaceRect = {
			cvRound(maxLoc.x * mTemplateScaleInv),
			cvRound(maxLoc.y * mTemplateScaleInv),
			cvRound(faceTpl.cols * mTemplateScaleInv),
			cvRound(faceTpl.rows * mTemplateScaleInv)
		};

		const cv::Rect screenRect(0, 0, iImage->GetWidth(), iImage->GetHeight());
		oFaceRect = oFaceRect & screenRect;

		return oFaceRect.area() > 0;
	}

	void UserManager::RemoveInactiveUsers(bool iForceToDelete)
	{
		if (mUsers.empty()) return;

		std::vector<int> userIDs;
		for (const auto& user : mUsers)
		{
			if (!user->IsActive())
			{
				const long long diff = std::llabs(fw::get_current_time() - user->GetLastUpdateTs());
				if (iForceToDelete || (diff > mUserAwaySec * 1000.0F))
					userIDs.push_back(user->GetUserId());
			}
		}

		for (auto& uid : userIDs)
		{
			auto itIU = std::find_if(mUsers.begin(), mUsers.end(), [&](const User::Shared& obj)
			{
				return obj->GetUserId() == uid;
			});

			if (itIU != mUsers.end())
			{
				LOG(INFO) << "User(" << uid << ") has been deleted completely.";
				mUsers.erase(itIU);
			}
		}
	}

	std::size_t UserManager::GetActiveUserSize() const
	{
		std::size_t size = 0U;

		for (const auto& user : mUsers)
		{
			if (user->IsActive())
				size++;
		}

		return size;
	}

	fw::ErrorCode UserManager::OnCommandArrived(fw::Message::Shared iMessage)
	{
		return fw::ErrorCode::OK;
	}
}
