#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"
#include "Framework/Stopwatch.h"

#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "User/User.h"
#include <memory>

namespace face
{
  class UserManager : public fw::Module,
                      public fw::Port<std::shared_ptr<ActiveUsersMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<RoiMessage>)>
  {
  public:

    UserManager() = default;

    virtual ~UserManager() = default;

    std::shared_ptr<ActiveUsersMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<RoiMessage> iDetections) override;

    void Clear() override;

    std::size_t GetActiveUserSize() const;

    inline std::size_t GetMaxUsers() const
    {
      return mMaxUsers;
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void PreprocessUsers();

    void ProcessDetections(std::shared_ptr<RoiMessage> iDetections);

    void TrackUsers(std::shared_ptr<ImageMessage> iImage);

    void PostprocessUsers();

    void MergeDetectionsAndUsers(std::vector<cv::Rect>& ioFaceROIs);

    bool MatchTemplate(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<User> ioUser, cv::Rect& oFaceRect);

    void RemoveInactiveUsers(bool forceToDelete = false);

    std::vector<std::shared_ptr<User>> mUsers; ///< The vector storing all users
    fw::Stopwatch mRemoveSW;
    fw::Timestamp mTimestamp;
    int mLastUserID = 0;

    cv::Size mMinFaceSize;
    cv::Size mMaxFaceSize;

    std::size_t mMaxUsers = 1U;
    float mUserOverlap = 0.2F;
    float mUserAwaySec = 15.0F;
    float mTemplateScale = 1.0f;
    float mTemplateScaleInv = 1.0f;
  };
}
