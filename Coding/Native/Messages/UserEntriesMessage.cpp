#include "UserEntriesMessage.h"

namespace face
{
	UserEntriesMessage::UserEntriesMessage(const EntryMap& iEntryMap, unsigned iFrameId, long long iTimestamp) :
		Message(iFrameId, iTimestamp),
		mEntryMap(iEntryMap)
	{
	}
}
