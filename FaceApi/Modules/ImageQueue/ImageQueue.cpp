#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/ImageQueue/ImageQueue.h"

#include "Framework/Text.h"

#include <cstdint>
#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/highgui/highgui.hpp>

namespace face
{
  ImageQueue::ImageQueue() :
    mQueue("ImageQueue", 12.0F, 10, fw::Milliseconds(500.0))
  {
  }

  fw::ErrorCode ImageQueue::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "samplingFPS", value))
        mQueue.SetSamplingFPS(fw::str::convert_to_number<float>(value));

      if (fw::get_value(iSettings, "bound", value))
        mQueue.SetBound(fw::str::convert_to_number<int>(value));

      if (fw::get_value(iSettings, "thresholdMS", value))
        mQueue.SetTimestampFiltering(fw::Milliseconds(fw::str::convert_to_number<double>(value)));
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ImageQueue::Push(const cv::Mat& iFrame, uint32_t iFrameId, fw::Timestamp iTimestamp)
  {
    if (iFrame.empty())
    {
      return fw::ErrorCode::BadParam;
    }

    // A size change Clear()s every module, so leave announcing it to Main().
    {
      std::lock_guard<std::mutex> lock(mSizeMutex);

      if (mImageSize != iFrame.size())
      {
        mImageSize = iFrame.size();
        mPendingSizeChange = true;

        LOG(INFO) << "Image size has been changed to: " << mImageSize;
      }
    }

    // The id and the timestamp come from the camera, they are not re-stamped here
    std::shared_ptr<ImageMessage> message = std::make_shared<ImageMessage>(iFrame, iFrameId, iTimestamp);

    // Stamped before the push: afterwards the message may already belong to the graph thread.
    // This frame is counted in, hence the +1 on a size read before it went on.
    const MessageQueue::Statistics stats = GetQueueStatistics();
    message->SetQueueData(stats.size + 1, stats.samplingFPS, stats.bound);

    // Never block the camera thread: dropping a frame beats stalling the capture.
    return mQueue.TryPush(message);
  }

  std::shared_ptr<ImageMessage> ImageQueue::Main()
  {
    DrainCommands();

    std::shared_ptr<ImageMessage> image;
    if (mQueue.TryPop(image) != fw::ErrorCode::OK) image = nullptr;

    if (image)
    {
      mLastFrameId = image->GetFrameId();
      mLastTimestamp = image->GetTimestamp();
    }

    // After the pop so this frame survives, before returning so modules are cleared.
    NotifyPendingSizeChange();

    return image;
  }

  void ImageQueue::NotifyPendingSizeChange()
  {
    cv::Size imageSize;

    {
      std::lock_guard<std::mutex> lock(mSizeMutex);

      if (!mPendingSizeChange) return;

      mPendingSizeChange = false;
      imageSize = mImageSize;
    }

    if (mAnnounceSizeChange) mAnnounceSizeChange(imageSize);
  }
}
