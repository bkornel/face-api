#include "Messages/CommandMessage.h"

namespace face
{
  CommandMessage::CommandMessage(Type iType, unsigned iFrameId, long long iTimestamp) :
    fw::Message(iFrameId, iTimestamp),
    mType(iType)
  {
  }
}
