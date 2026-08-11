#include "Messages/ImageSizeChangedMessage.h"

namespace face
{
  ImageSizeChangedMessage::ImageSizeChangedMessage(const cv::Size& iSize, unsigned iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp),
    mSize(iSize)
  {
  }
}
