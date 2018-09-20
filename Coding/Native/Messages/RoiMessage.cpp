#include "Messages/RoiMessage.h"

namespace face
{
	RoiMessage::RoiMessage(const std::vector<cv::Rect>& iROIs, unsigned iFrameId, long long iTimestamp) :
		Message(iFrameId, iTimestamp),
		mROIs(iROIs)
	{
	}
}
