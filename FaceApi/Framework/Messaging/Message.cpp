#include "Framework/Messaging/Message.h"

namespace fw
{
  Message::Message(unsigned iFrameId, Timestamp iTimestamp) :
    mFrameId(iFrameId),
    mTimestamp(iTimestamp)
  {
  }
}
