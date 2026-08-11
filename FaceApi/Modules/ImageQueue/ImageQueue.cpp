#include "Modules/ImageQueue/ImageQueue.h"
#include "Messages/ImageArrivedMessage.h"
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

  void ImageQueue::OnCommand(fw::Message::Shared iMessage)
  {
    Module::OnCommand(iMessage);

    ImageArrivedMessage::Shared imageArrived = std::dynamic_pointer_cast<ImageArrivedMessage>(iMessage);
    if (imageArrived && !imageArrived->IsEmpty())
    {
      Push(imageArrived->GetFrame());
    }
  }

  fw::ErrorCode ImageQueue::Push(const cv::Mat& iFrame)
  {
    if (iFrame.empty())
    {
      return fw::ErrorCode::BadParam;
    }

    const long long timestamp = fw::get_current_time();

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

    ImageMessage::Shared message = std::make_shared<ImageMessage>(iFrame, mPushFrameId, timestamp);
    message->SetQueueData(GetQueueSize(), GetSamplingFPS(), GetBound());

    // Never block the camera thread: dropping a frame beats stalling the capture.
    const fw::ErrorCode result = mQueue.TryPush(message);

    if (result == fw::ErrorCode::OK)
    {
      mPushFrameId++;
    }

    return result;
  }

  ImageMessage::Shared ImageQueue::Main(unsigned /*iTickNumber*/)
  {
    std::tuple<ImageMessage::Shared> framePool;
    const bool hasFrame = (mQueue.TryPop(framePool) == fw::ErrorCode::OK);

    // After the pop so this frame survives, before returning so modules are cleared.
    NotifyPendingSizeChange();

    if (!hasFrame)
    {
      return nullptr;
    }

    ImageMessage::Shared image = std::get<0>(framePool);
    mLastFrameId = image->GetFrameId();
    mLastTimestamp = image->GetTimestamp();

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

    sCommand.Raise(std::make_shared<ImageSizeChangedMessage>(imageSize, mPushFrameId, fw::get_current_time()));
  }
}
