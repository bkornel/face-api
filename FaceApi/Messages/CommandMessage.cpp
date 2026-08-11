#include "Messages/CommandMessage.h"

namespace face
{
  CommandMessage::CommandMessage(Type iType, unsigned iFrameId, fw::Timestamp iTimestamp) :
    fw::Message(iFrameId, iTimestamp),
    mType(iType)
  {
  }
}
