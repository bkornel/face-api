#include "Messages/ImageSizeChangedMessage.h"

namespace face
{
	ImageSizeChangedMessage::ImageSizeChangedMessage(const cv::Size& iSize, unsigned iFrameId, long long iTimestamp) :
		Message(iFrameId, iTimestamp),
		mSize(iSize)
	{
	}
}
