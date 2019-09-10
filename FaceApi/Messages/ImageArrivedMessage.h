#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

namespace face
{
  class ImageArrivedMessage :
    public fw::Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(ImageArrivedMessage);

    ImageArrivedMessage(const cv::Mat& iFrame, unsigned iFrameId, long long iTimestamp);

    virtual ~ImageArrivedMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const ImageArrivedMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mFrame.empty();
    }

    inline const cv::Mat& GetFrame() const
    {
      return mFrame;
    }

    inline int GetWidth() const
    {
      return mFrame.cols;
    }

    inline int GetHeight() const
    {
      return mFrame.rows;
    }

  private:
    cv::Mat mFrame;
  };

  inline std::ostream& operator<< (std::ostream& ioStream, const ImageArrivedMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Width: " << iMessage.GetWidth() << ", Height: " << iMessage.GetHeight();
    return ioStream;
  }
}
