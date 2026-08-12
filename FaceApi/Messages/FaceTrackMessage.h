#pragma once

#include "Framework/Messaging/Message.h"
#include "User/TrackedFace.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The faces the tracker follows on one frame, as values. What the shape fit and
  /// everything after it know about a face starts from here.
  class FaceTrackMessage : public fw::Message
  {
  public:

    using TrackVector = std::vector<TrackedFace>;

    FaceTrackMessage(TrackVector iTracks, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mTracks(std::move(iTracks))
    {
    }

    virtual ~FaceTrackMessage() = default;

    inline bool IsEmpty() const
    {
      return mTracks.empty();
    }

    inline std::size_t GetSize() const
    {
      return mTracks.size();
    }

    inline const TrackVector& GetTracks() const
    {
      return mTracks;
    }

  private:
    TrackVector mTracks;
  };
}
