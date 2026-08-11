#pragma once

#include "Framework/Message.h"
#include "User/User.h"
#include <memory>

namespace face
{
  class ActiveUsersMessage : public fw::Message
  {
  public:

    typedef std::vector<std::shared_ptr<User>> UserVector;

    ActiveUsersMessage(const UserVector& iActiveUsers, unsigned iFrameId, long long iTimestamp);

    virtual ~ActiveUsersMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const ActiveUsersMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mActiveUsers.empty();
    }

    inline std::size_t GetSize() const
    {
      return mActiveUsers.size();
    }

    inline const UserVector& GetActiveUsers() const
    {
      return mActiveUsers;
    }

  private:
    UserVector mActiveUsers; ///< The vector storing the active viewers
  };

  inline std::ostream& operator<<(std::ostream& ioStream, const ActiveUsersMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Size of users: " << iMessage.GetSize();
    return ioStream;
  }
}
