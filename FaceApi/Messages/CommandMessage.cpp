#include "Messages/CommandMessage.h"

namespace face
{
  CommandMessage::CommandMessage(Type iType, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    CommandMessage(iType, false, iFrameId, iTimestamp)
  {
  }

  CommandMessage::CommandMessage(Type iType, bool iFlag, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    fw::Message(iFrameId, iTimestamp),
    mType(iType),
    mFlag(iFlag)
  {
  }
}
