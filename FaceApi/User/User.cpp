#include "User/User.h"
#include "User/UserDispatcher.hpp"

#include "Common/Configuration.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/imgproc/types_c.h>

namespace face
{
  User::User(const cv::Rect& iFaceRect, int iUserId, long long iTimestamp) :
    mUserId(iUserId),
    mCreationTs(iTimestamp)
  {
    SetDetectionData(iFaceRect, iTimestamp);
  }

  bool User::AcceptDispatcher(UserDispatcher& ioDispatcher)
  {
    return ioDispatcher.Dispatch(*this);
  }

  void User::SetDetectionData(const cv::Rect& iFaceRect, long long iTimestamp)
  {
    mLastDetectionTs = iTimestamp;
    SetFaceRect(iFaceRect);
    SetStatus(Status::Detected);
  }

  void User::SetStatus(Status iStatus)
  {
    if (iStatus == mStatus) return;

    mStatus = iStatus;

    if (mStatus == Status::Detected)
    {
      LOG(INFO) << "User(" << mUserId << ") is detected";
    }
    else if (mStatus == Status::ToBeTracked)
    {
      LOG(INFO) << "User(" << mUserId << ") can be tracked";
    }
    else if (mStatus == Status::Tracked)
    {
      LOG_EVERY_N(10, INFO) << "User(" << mUserId << ") is tracked";
    }
    else
    {
      LOG(INFO) << "User(" << mUserId << ") is inactivated";
    }
  }
}
