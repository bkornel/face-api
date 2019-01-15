#include "Message.h"

namespace fw
{
	Message::Message(unsigned iFrameId, long long iTimestamp) :
		mFrameId(iFrameId),
		mTimestamp(iTimestamp)
	{
	}
}