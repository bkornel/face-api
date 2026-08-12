#pragma once

#include "Framework/TimeExtensions.h"
#include "Model/FaceModel.h"
#include "User/UserData.hpp"

#include <clm/CLM.h>
#include <opencv2/core/core.hpp>
#include <map>

namespace face
{
  class User : public UserData
  {
  public:

    enum class Status
    {
      Detected = 0,
      ToBeTracked,
      Tracked,
      Inactive
    };

    User(const cv::Rect& iFaceRect, int iUserId, fw::Timestamp iTimestamp);

    User(const User& iOther) = default;

    virtual ~User() = default;

    inline bool IsActive() const
    {
      return mStatus != Status::Inactive;
    }

    inline bool IsDetected() const
    {
      return mStatus == Status::Detected;
    }

    inline int GetUserId() const
    {
      return mUserId;
    }

    inline fw::Timestamp GetCreationTs() const
    {
      return mCreationTs;
    }

    inline fw::Timestamp GetLastUpdateTs() const
    {
      return mLastUpdateTs;
    }

    inline fw::Timestamp GetLastDetectionTs() const
    {
      return mLastDetectionTs;
    }

    inline void SetLastUpdateTs(fw::Timestamp iTimestamp)
    {
      mLastUpdateTs = iTimestamp;
    }

    void SetDetectionData(const cv::Rect& iFaceRect, fw::Timestamp iTimestamp);
    void SetStatus(Status iStatus);

  private:
    const int mUserId = 0;
    const fw::Timestamp mCreationTs;

    fw::Timestamp mLastUpdateTs;
    fw::Timestamp mLastDetectionTs;

    Status mStatus = Status::Detected;
  };
}
