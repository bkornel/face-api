#include "Messages/ImageArrivedMessage.h"

namespace face
{
  ImageArrivedMessage::ImageArrivedMessage(const cv::Mat& iFrame, unsigned iFrameId, long long iTimestamp) :
    fw::Message(iFrameId, iTimestamp),
    mFrame(iFrame)
  {
  }
}
