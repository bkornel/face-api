#pragma once


#include "Framework/MessageQueue.hpp"
#include "Framework/Module.h"
#include "Framework/Port.hpp"

#include "Messages/ImageMessage.h"

#include <string>

namespace face
{
  class ImageQueue :
    public fw::Module,
    public fw::Port<ImageMessage::Shared(unsigned)>
  {
    using MessageQueue = fw::MessageQueue<ImageMessage::Shared>;

  public:
    FW_DEFINE_SMART_POINTERS(ImageQueue);

    ImageQueue();

    virtual ~ImageQueue() = default;

    fw::ErrorCode Push(const cv::Mat& iFrame);

    ImageMessage::Shared Main(unsigned iTickNumber) override;

    void Clear() override
    {
      mPushFrameId = 0U;
      mQueue.Clear();
    }

    inline const cv::Size& GetImageSize() const
    {
      return mImageSize;
    }

    inline unsigned GetLastFrameId() const
    {
      return mLastFrameId;
    }

    inline long long GetLastTimestamp() const
    {
      return mLastTimestamp;
    }

    inline int GetQueueSize() const
    {
      return mQueue.GetSize();
    }

    inline float GetSamplingFPS() const
    {
      return mQueue.GetSamplingFPS();
    }

    inline int GetBound() const
    {
      return mQueue.GetBound();
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    unsigned mPushFrameId = 0U;
    unsigned mLastFrameId = 0U;		///< Holds the ID of the last image frame.
    long long mLastTimestamp = 0;
    MessageQueue mQueue;			    ///< Queue for handling the frames
    cv::Size mImageSize;
  };
}
