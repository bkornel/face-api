#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

namespace face
{
  class RoiMessage :
    public fw::Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(RoiMessage);

    RoiMessage(const std::vector<cv::Rect>& iROIs, const cv::Size& iMinRoiSize, const cv::Size& iMaxRoiSize, unsigned iFrameId, long long iTimestamp);

    virtual ~RoiMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const RoiMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mROIs.empty();
    }

    inline std::size_t GetSize() const
    {
      return mROIs.size();
    }

    inline const std::vector<cv::Rect>& GetROIs() const
    {
      return mROIs;
    }

    inline const cv::Size& GetMinRoiSize() const
    {
      return mMinRoiSize;
    }

    inline const cv::Size& GetMaxRoiSize() const
    {
      return mMaxRoiSize;
    }

  private:
    std::vector<cv::Rect> mROIs;
    cv::Size mMinRoiSize;
    cv::Size mMaxRoiSize;
  };

  inline std::ostream& operator<< (std::ostream& ioStream, const RoiMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Number of ROIs: " << iMessage.GetSize() <<
      ", min ROI size: " << iMessage.GetMinRoiSize() <<
      ", max ROI size: " << iMessage.GetMaxRoiSize();
    return ioStream;
  }
}
