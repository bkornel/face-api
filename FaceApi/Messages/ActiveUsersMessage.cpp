#include "Messages/ActiveUsersMessage.h"

namespace face
{
  ActiveUsersMessage::ActiveUsersMessage(const UserVector& iActiveUsers, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp)
  {
    for (const auto& user : iActiveUsers)
    {
      if (user && user->IsActive())
        mActiveUsers.emplace_back(user);
    }
  }

}
