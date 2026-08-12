#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include "Modules/UserManager/UserManager.h"
#include "Messages/CommandMessage.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <iomanip>

namespace face
{
  fw::ErrorCode UserManager::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      // At least one: a graph that may track nobody at all has no reason to run
      if (fw::get_value(iSettings, "maxUsers", value))
        mMaxUsers = static_cast<std::size_t>((std::max)(fw::str::convert_to_number<int>(value), 1));

      if (fw::get_value(iSettings, "userOverlap", value))
        mUserOverlap = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "userAwaySec", value))
        mUserAwaySec = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "templateScale", value))
      {
        mTemplateScale = fw::str::convert_to_number<float>(value);
        mTemplateScale = (std::max)((std::min)(mTemplateScale, 1.0F), 0.2F);
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
    mRemoveSW.Reset();

    mMinFaceSize = mMaxFaceSize = { 0, 0 };
  }

  std::shared_ptr<ActiveUsersMessage> UserManager::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<RoiMessage> iDetections)
  {
    DrainCommands();

    if (!iImage || iImage->IsEmpty()) return nullptr;

    FACE_PROFILER(User_Manager);

    const unsigned frameId = iImage->GetFrameId();
    mTimestamp = iImage->GetTimestamp();

    // Active users to inactive and set is-detected to false
    PreprocessUsers();

    // Merge users and detections and create new users
    ProcessDetections(iDetections);

    // Track users by their faces
    TrackUsers(iImage);

    // Remove old user history entries and inactive users
    PostprocessUsers();

    if (GetMaxUsers() != GetActiveUserSize())
    {
      Publish(std::make_shared<CommandMessage>(CommandMessage::Type::RunFaceDetection, frameId, mTimestamp));
    }

    return GetActiveUserSize() > 0 ? std::make_shared<ActiveUsersMessage>(mUsers, frameId, mTimestamp) : nullptr;
  }

  void UserManager::PreprocessUsers()
  {
    for (const auto& user : mUsers)
    {
      const auto& facerect = user->GetFaceRect();

      const bool inactivate =
        // Must be active
        user->IsActive() && (
                              // Minimal resolution
                              (!mMinFaceSize.empty() && ((facerect.width < mMinFaceSize.width) || (facerect.height < mMinFaceSize.height))) ||
                              // Maximal resolution
                              (!mMaxFaceSize.empty() && ((facerect.width > mMaxFaceSize.width) || (facerect.height > mMaxFaceSize.height))) ||
                              // Detected a long time ago
                              (fw::elapsed(user->GetLastDetectionTs(), mTimestamp) > fw::Milliseconds(mUserAwaySec * 1000.0F))
                            );

      if (inactivate)
        user->SetStatus(User::Status::Inactive);

      // Set the status to-be-tracked after detection and before tracking
      if (user->IsDetected())
        user->SetStatus(User::Status::ToBeTracked);
    }
  }

  void UserManager::ProcessDetections(std::shared_ptr<RoiMessage> iDetections)
  {
    if (!iDetections || iDetections->IsEmpty()) return;

    std::vector<cv::Rect> faceROIs = iDetections->GetROIs();
    mMinFaceSize = iDetections->GetMinRoiSize();
    mMaxFaceSize = iDetections->GetMaxRoiSize();

    // Checking the overlap between detector's rectangles and users
    MergeDetectionsAndUsers(faceROIs);

    // Add new users
    for (const auto& r : faceROIs)
    {
      if (GetActiveUserSize() >= GetMaxUsers()) break;

      mUsers.emplace_back(std::make_shared<User>(r, mLastUserID, mTimestamp));
      LOG(INFO) << "New user has been recognized, Welcome User(" << mLastUserID << ")!";
      mLastUserID++;
    }
  }

  void UserManager::TrackUsers(std::shared_ptr<ImageMessage> iImage)
  {
    FACE_PROFILER(Track_Users);

    const cv::Mat frameGray = iImage->GetFrameGray();
    const cv::Rect screenRect(0, 0, frameGray.cols, frameGray.rows);

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

      // Only the slice uses the clipped rectangle: assigning it back would recompute the
      // offset the shape model shifts by, and flatten it whenever the clip changes nothing.
      const cv::Rect faceRect = user->GetFaceRect() & screenRect;

      if (faceRect.area() <= 0)
      {
        user->SetStatus(User::Status::Inactive);
        continue;
      }

      user->SetFaceTemplate(frameGray(faceRect));
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
      // Taking an inactive user back makes it active, so it needs a free slot. Asked per
      // user, not per detection: the answer cannot change while this user is matched.
      if (!user->IsActive() && GetActiveUserSize() >= GetMaxUsers())
        continue;

      // A copy: SetDetectionData() moves the rectangle, and the detections are matched
      // against where the user was last seen.
      const cv::Rect userFaceRect = user->GetFaceRect();

      for (auto fr = ioFaceROIs.begin(); fr != ioFaceROIs.end();)
      {
        if (fw::overlap_ratio(userFaceRect, *fr) > mUserOverlap)
        {
          // This also sets the status to detected, so the user counts as active from here
          user->SetDetectionData(*fr, mTimestamp);

          fr = ioFaceROIs.erase(fr);
        }
        else
        {
          ++fr;
        }
      }
    }
  }

  bool UserManager::MatchTemplate(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<User> ioUser, cv::Rect& oFaceRect)
  {
    CV_DbgAssert(mTemplateScale > 0.0F && mTemplateScale <= 1.0F);

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
        const fw::Milliseconds idle = fw::elapsed_since(user->GetLastUpdateTs());
        if (iForceToDelete || (idle > fw::Milliseconds(mUserAwaySec * 1000.0F)))
          userIDs.emplace_back(user->GetUserId());
      }
    }

    for (auto& uid : userIDs)
    {
      auto itIU = std::find_if(mUsers.begin(), mUsers.end(), [&](const std::shared_ptr<User>& obj) {
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
}
