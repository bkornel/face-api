#pragma once

#include "Framework/Messaging/Message.h"
#include "Messages/ActiveUsersMessage.h"
#include "User/User.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace face
{
  /// @brief The users of one finished frame, as data nobody may change any more.
  ///
  /// ActiveUsersMessage carries the live users the tracking keeps writing to, and stays inside
  /// the graph. This one is what leaves it: the users are copied on the way in and handed out
  /// as const, so a reader on another thread cannot be looking at a user while it is being
  /// updated. The two being separate types is also what stops a settings file from wiring a
  /// reader straight to the tracking - the ports no longer fit together.
  class UserSnapshotMessage : public fw::Message
  {
  public:

    using UserVector = std::vector<std::shared_ptr<const User>>;

    UserSnapshotMessage(const ActiveUsersMessage::UserVector& iActiveUsers, uint32_t iFrameId, fw::Timestamp iTimestamp);

    virtual ~UserSnapshotMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const UserSnapshotMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mUsers.empty();
    }

    inline std::size_t GetSize() const
    {
      return mUsers.size();
    }

    inline const UserVector& GetUsers() const
    {
      return mUsers;
    }

  private:
    UserVector mUsers;
  };

  inline std::ostream& operator<<(std::ostream& ioStream, const UserSnapshotMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Size of users: " << iMessage.GetSize();
    return ioStream;
  }
}
