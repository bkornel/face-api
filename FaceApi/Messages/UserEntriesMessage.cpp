#include "UserEntriesMessage.h"

namespace face
{
  UserEntriesMessage::UserEntriesMessage(const EntryMap& iEntryMap, unsigned iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp),
    mEntryMap(iEntryMap)
  {
  }
}
