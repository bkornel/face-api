#include "Messages/UserSnapshotMessage.h"

namespace face
{
  UserSnapshotMessage::UserSnapshotMessage(const ActiveUsersMessage::UserVector& iActiveUsers, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp)
  {
    mUsers.reserve(iActiveUsers.size());

    for (const auto& user : iActiveUsers)
    {
      // Copied, not shared: UserData's copy clones the matrices, so the result owns
      // everything it holds and the tracking can carry on writing to the original.
      if (user && user->IsActive())
        mUsers.emplace_back(std::make_shared<const User>(*user));
    }
  }
}
