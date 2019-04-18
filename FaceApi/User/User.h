#pragma once

#include "Common/ShapeUtil.h"
#include "User/UserData.hpp"

#include <clm/CLM.h>
#include <opencv2/core/core.hpp>
#include <map>

namespace face
{
  class UserDispatcher;

  class User :
    public UserData
  {
  public:
    FW_DEFINE_SMART_POINTERS(User);

    enum class Status
    {
      Detected = 0,
      ToBeTracked,
      Tracked,
      Inactive
    };

    User(const cv::Rect& iFaceRect, int iUserId, long long iTimestamp);

    virtual ~User() = default;

    bool AcceptDispatcher(UserDispatcher& ioDispatcher);

    inline bool IsActive() const { return mStatus != Status::Inactive; }
    inline bool IsDetected() const { return mStatus == Status::Detected; }

    inline int GetUserId() const { return mUserId; }
    inline long long GetCreationTs() const { return mCreationTs; }
    inline long long GetLastUpdateTs() const { return mLastUpdateTs; }
    inline long long GetLastDetectionTs() const { return mLastDetectionTs; }

    inline void SetLastUpdateTs(long long iTimestamp) { mLastUpdateTs = iTimestamp; }

    void SetDetectionData(const cv::Rect& iFaceRect, long long iTimestamp);
    void SetStatus(Status iStatus);

  private:
    const int mUserId = 0;
    const long long mCreationTs = 0;

    long long mLastUpdateTs = 0;
    long long mLastDetectionTs = 0;

    Status mStatus = Status::Detected;
  };
}
