#pragma once

#include "Framework/Stopwatch.h"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"
#include "Messages/ActiveUsersMessage.h"
#include "Modules/General/ModuleWithPort.hpp"
#include "User/User.h"

namespace face
{
  class UserManager :
    public ModuleWithPort<ActiveUsersMessage::Shared(ImageMessage::Shared, RoiMessage::Shared)>
  {
  public:
    UserManager() = default;

    virtual ~UserManager() = default;

    ActiveUsersMessage::Shared Main(ImageMessage::Shared iImage, RoiMessage::Shared iDetections) override;

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

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void PreprocessUsers();

    void ProcessDetections(RoiMessage::Shared iDetections);

    void TrackUsers(ImageMessage::Shared iImage);

    void PostprocessUsers();

    void MergeDetectionsAndUsers(std::vector<cv::Rect>& ioFaceROIs);

    bool MatchTemplate(ImageMessage::Shared iImage, User::Shared ioUser, cv::Rect& oFaceRect);

    void RemoveInactiveUsers(bool forceToDelete = false);

    void OnCommand(fw::Message::Shared iMessage) override;

    std::vector<User::Shared> mUsers;   ///< The vector storing all users
    fw::Stopwatch mRemoveSW;
    long long mTimestamp = 0;

    cv::Size mMinFaceSize;
    cv::Size mMaxFaceSize;

    int mMaxUsers = 1;
    float mUserOverlap = 0.2F;
    float mUserAwaySec = 15.0F;
    float mTemplateScale = 1.0f;
    float mTemplateScaleInv = 1.0f;
  };
}
