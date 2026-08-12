#pragma once

#include "Framework/Messaging/Message.h"
#include "User/TrackedFace.h"
#include "User/UserData.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The results computed for one tracked face so far. Travels by value: each stage
  /// copies the entries, fills in its own fields and passes the message on, so no two
  /// modules ever hold the same mutable object.
  struct FaceData
  {
    TrackedFace track;
    UserData data;
  };

  class FaceDataMessage : public fw::Message
  {
  public:

    using FaceDataVector = std::vector<FaceData>;

    FaceDataMessage(FaceDataVector iEntries, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mEntries(std::move(iEntries))
    {
    }

    virtual ~FaceDataMessage() = default;

    inline bool IsEmpty() const
    {
      return mEntries.empty();
    }

    inline std::size_t GetSize() const
    {
      return mEntries.size();
    }

    inline const FaceDataVector& GetEntries() const
    {
      return mEntries;
    }

  private:
    FaceDataVector mEntries;
  };
}
