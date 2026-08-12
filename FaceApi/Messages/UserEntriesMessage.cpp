#include "UserEntriesMessage.h"

namespace face
{
  UserEntriesMessage::UserEntriesMessage(const EntryMap& iEntryMap, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp),
    mEntryMap(iEntryMap)
  {
  }
}
