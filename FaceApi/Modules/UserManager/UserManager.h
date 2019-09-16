#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"

#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "User/User.h"

namespace face
{
  class UserManager :
    public fw::Module,
    public fw::Port<ActiveUsersMessage::Shared(ImageMessage::Shared, RoiMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(UserManager);

    UserManager() = default;

    virtual ~UserManager() = default;

    ActiveUsersMessage::Shared Main(ImageMessage::Shared iImage, RoiMessage::Shared iDetections) override;

    void Clear() override;

    std::size_t GetActiveUserSize() const;

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
