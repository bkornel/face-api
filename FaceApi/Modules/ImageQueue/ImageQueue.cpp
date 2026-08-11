#include "Modules/ImageQueue/ImageQueue.h"
#include "Messages/ImageSizeChangedMessage.h"

#include "Framework/UtilOCV.h"
#include "Framework/UtilString.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/highgui/highgui.hpp>

namespace face
{
  ImageQueue::ImageQueue() :
    mQueue("ImageQueue", 12.0F, 10, 500)
  {
  }

  fw::ErrorCode ImageQueue::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::ocv::get_value(iSettings, "samplingFPS", value))
        mQueue.SetSamplingFPS(fw::str::convert_to_number<float>(value));

      if (fw::ocv::get_value(iSettings, "bound", value))
        mQueue.SetBound(fw::str::convert_to_number<int>(value));

      if (fw::ocv::get_value(iSettings, "thresholdMS", value))
        mQueue.SetTimestampFiltering(fw::str::convert_to_number<int>(value));
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ImageQueue::Push(const cv::Mat& iFrame, unsigned iFrameId, long long iTimestamp)
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
    ImageMessage::Shared message = std::make_shared<ImageMessage>(iFrame, iFrameId, iTimestamp);

    // Counting this frame in, since after the push the message may already belong to the
    // graph thread and writing to it would race
    message->SetQueueData(GetQueueSize() + 1, GetSamplingFPS(), GetBound());

    // Never block the camera thread: dropping a frame beats stalling the capture.
    return mQueue.TryPush(message);
  }

  ImageMessage::Shared ImageQueue::Main(unsigned /*iTickNumber*/)
  {
    DrainCommands();

    std::tuple<ImageMessage::Shared> framePool;
    ImageMessage::Shared image =
      (mQueue.TryPop(framePool) == fw::ErrorCode::OK) ? std::get<0>(framePool) : nullptr;

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

    Publish(std::make_shared<ImageSizeChangedMessage>(imageSize, mLastFrameId, mLastTimestamp));
  }
}
