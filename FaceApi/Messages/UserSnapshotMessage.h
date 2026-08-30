#pragma once

#include "Framework/Messaging/Message.h"
#include "User/User.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The users of one finished frame, as data nobody may change any more. The user
  /// manager composes them from the raw results; everything reading them - the visualizer,
  /// the history, the host application - gets them as const and cannot be looking at a user
  /// while it is being written.
  class UserSnapshotMessage : public fw::Message
  {
  public:

    using UserVector = std::vector<std::shared_ptr<const User>>;

    UserSnapshotMessage(UserVector iUsers, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mUsers(std::move(iUsers))
    {
    }

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
