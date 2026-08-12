#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ImageMessage.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace face
{
  class ImageQueue : public fw::Module,
                     public fw::Port<std::shared_ptr<ImageMessage>(uint32_t)>
  {
    using MessageQueue = fw::MessageQueue<std::shared_ptr<ImageMessage>>;

  public:

    ImageQueue();

    virtual ~ImageQueue() = default;

    fw::ErrorCode Push(const cv::Mat& iFrame, uint32_t iFrameId, fw::Timestamp iTimestamp);

    std::shared_ptr<ImageMessage> Main(uint32_t iTickNumber) override;

    void Clear() override
    {
      mQueue.Clear();
    }

    inline cv::Size GetImageSize() const
    {
      std::lock_guard<std::mutex> lock(mSizeMutex);
      return mImageSize;
    }

    inline uint32_t GetLastFrameId() const
    {
      return mLastFrameId;
    }

    inline fw::Timestamp GetLastTimestamp() const
    {
      return mLastTimestamp;
    }

    inline MessageQueue::Statistics GetQueueStatistics() const
    {
      return mQueue.GetStatistics();
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void NotifyPendingSizeChange();

    std::atomic<uint32_t> mLastFrameId{ 0U }; ///< Holds the ID of the last image frame.
    std::atomic<fw::Timestamp> mLastTimestamp{};

    MessageQueue mQueue; ///< Queue for handling the frames

    mutable std::mutex mSizeMutex;
    cv::Size mImageSize;
    bool mPendingSizeChange = false;
  };
}
