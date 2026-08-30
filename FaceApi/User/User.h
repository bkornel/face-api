#pragma once

#include "Framework/TimeExtensions.h"
#include "User/TrackedFace.h"
#include "User/UserData.hpp"

#include <opencv2/core/core.hpp>

namespace face
{
  /// @brief One user of one finished frame: the tracker's view of the face and the results
  /// computed for it, composed into a single record. Built once by the user manager and
  /// read-only afterwards - nothing in the pipeline writes to a User.
  class User : public UserData
  {
  public:

    using Status = TrackStatus;

    User(const TrackedFace& iTrack, const UserData& iData) :
      UserData(iData),
      mTrack(iTrack)
    {
    }

    User(const User& iOther) = default;

    virtual ~User() = default;

    inline bool IsActive() const
    {
      return mTrack.status != TrackStatus::Inactive;
    }

    inline bool IsDetected() const
    {
      return mTrack.status == TrackStatus::Detected;
    }

    inline int GetUserId() const
    {
      return mTrack.trackId;
    }

    inline TrackStatus GetStatus() const
    {
      return mTrack.status;
    }

    inline fw::Timestamp GetCreationTs() const
    {
      return mTrack.creationTs;
    }

    inline fw::Timestamp GetLastUpdateTs() const
    {
      return mTrack.lastUpdateTs;
    }

    inline fw::Timestamp GetLastDetectionTs() const
    {
      return mTrack.lastDetectionTs;
    }

  private:
    TrackedFace mTrack;
  };
}
