#include "Messages/ImageSizeChangedMessage.h"

namespace face
{
  ImageSizeChangedMessage::ImageSizeChangedMessage(const cv::Size& iSize, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp),
    mSize(iSize)
  {
  }
}
