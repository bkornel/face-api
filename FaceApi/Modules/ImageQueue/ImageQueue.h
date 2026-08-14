#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ImageMessage.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace face
{
  /// @brief Where pushed frames wait for the graph, and the source of the graph itself: it
  /// declares no input, so a frame's round starts with Trigger() and everything downstream
  /// follows from the message it pops.
  class ImageQueue : public fw::Module,
                     public fw::Port<std::shared_ptr<ImageMessage>()>
  {
    using MessageQueue = fw::MessageQueue<std::shared_ptr<ImageMessage>>;

  public:

    ImageQueue();

    virtual ~ImageQueue() = default;

    fw::ErrorCode Push(const cv::Mat& iFrame, uint32_t iFrameId, fw::Timestamp iTimestamp);

    /// @brief Sleeps until a frame is waiting, the queue closes or iTimeout passes, so the
    /// worker that drives the graph can block here instead of polling.
    /// @return true when a frame is waiting
    bool WaitForFrame(fw::Milliseconds iTimeout)
    {
      return mQueue.WaitForMessage(iTimeout);
    }

    /// @brief Wired by whoever builds the graph; called on the graph thread when the size
    /// of the incoming frames changes, after the change has been popped
    void SetSizeChangedAnnouncer(std::function<void(cv::Size)> iAnnouncer)
    {
      mAnnounceSizeChange = std::move(iAnnouncer);
    }

    std::shared_ptr<ImageMessage> Main() override;

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

    std::function<void(cv::Size)> mAnnounceSizeChange;

    mutable std::mutex mSizeMutex;
    cv::Size mImageSize;
    bool mPendingSizeChange = false;
  };
}
