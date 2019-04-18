#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

#include <map>
#include <mutex>

namespace face
{
  class ImageMessage :
    public fw::Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(ImageMessage);

    ImageMessage(const cv::Mat& iImage, unsigned iFrameId, long long iTimestamp);

    virtual ~ImageMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const ImageMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mFrames.first.empty();
    }

    inline const cv::Mat& GetFrameBGR() const
    {
      return mFrames.first;
    }

    const cv::Mat& GetFrameGray();

    const cv::Mat& GetResizedBGR(float iScaleFactor);

    const cv::Mat& GetResizedGray(float iScaleFactor);

    inline int GetWidth() const
    {
      return mFrames.first.cols;
    }

    inline int GetHeight() const
    {
      return mFrames.first.rows;
    }

    inline cv::Size GetSize() const
    {
      return mFrames.first.size();
    }

  private:
    using ImagePair = std::pair<cv::Mat, cv::Mat>; // BGR - Gray
    using ResizedImages = std::map<int, ImagePair>; // Key: width of the image (aspect ratio is fixed)

    static std::recursive_mutex sMutex;

    ImagePair mFrames;
    ResizedImages mResizedFrames;
  };

  inline std::ostream& operator<< (std::ostream& ioStream, const ImageMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Width: " << iMessage.GetWidth() << ", Height: " << iMessage.GetHeight();
    return ioStream;
  }
}
