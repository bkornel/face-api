#include "Messages/CommandMessage.h"

namespace face
{
  CommandMessage::CommandMessage(Type iType, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    fw::Message(iFrameId, iTimestamp),
    mType(iType)
  {
  }
}
