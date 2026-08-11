#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ImageMessage.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace face
{
  class ImageQueue : public fw::Module,
                     public fw::Port<std::shared_ptr<ImageMessage>(unsigned)>
  {
    using MessageQueue = fw::MessageQueue<std::shared_ptr<ImageMessage>>;

  public:

    ImageQueue();

    virtual ~ImageQueue() = default;

    fw::ErrorCode Push(const cv::Mat& iFrame, unsigned iFrameId, fw::Timestamp iTimestamp);

    std::shared_ptr<ImageMessage> Main(unsigned iTickNumber) override;

    void Clear() override
    {
      mQueue.Clear();
    }

    inline cv::Size GetImageSize() const
    {
      std::lock_guard<std::mutex> lock(mSizeMutex);
      return mImageSize;
    }

    inline unsigned GetLastFrameId() const
    {
      return mLastFrameId;
    }

    inline fw::Timestamp GetLastTimestamp() const
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

    void NotifyPendingSizeChange();

    std::atomic<unsigned> mLastFrameId{ 0U }; ///< Holds the ID of the last image frame.
    std::atomic<fw::Timestamp> mLastTimestamp{};

    MessageQueue mQueue; ///< Queue for handling the frames

    mutable std::mutex mSizeMutex;
    cv::Size mImageSize;
    bool mPendingSizeChange = false;
  };
}
