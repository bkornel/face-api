#include "Messages/ActiveUsersMessage.h"

namespace face
{
  ActiveUsersMessage::ActiveUsersMessage(const UserVector& iActiveUsers, unsigned iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp)
  {
    for (const auto& user : iActiveUsers)
    {
      if (user && user->IsActive())
        mActiveUsers.emplace_back(user);
    }
  }

}
