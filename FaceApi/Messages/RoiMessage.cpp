#include "Messages/RoiMessage.h"

namespace face
{
  RoiMessage::RoiMessage(const std::vector<cv::Rect>& iROIs, const cv::Size& iMinRoiSize, const cv::Size& iMaxRoiSize, uint32_t iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp),
    mROIs(iROIs),
    mMinRoiSize(iMinRoiSize),
    mMaxRoiSize(iMaxRoiSize)
  {
  }
}
