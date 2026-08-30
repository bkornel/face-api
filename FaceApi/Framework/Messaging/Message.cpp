#include "Framework/Messaging/Message.h"

namespace fw
{
  Message::Message(uint32_t iFrameId, Timestamp iTimestamp) :
    mFrameId(iFrameId),
    mTimestamp(iTimestamp)
  {
  }
}
