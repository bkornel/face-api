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

    if (mImageSize != iFrame.size())
    {
      mImageSize = iFrame.size();
      sCommand.Raise(std::make_shared<ImageSizeChangedMessage>(mImageSize, mPushFrameId, timestamp));

      LOG(INFO) << "Image size has been changed to: " << mImageSize;
    }

    ImageMessage::Shared message = std::make_shared<ImageMessage>(iFrame, mPushFrameId, timestamp);
    message->SetQueueData(GetQueueSize(), GetSamplingFPS(), GetBound());

    const fw::ErrorCode result = mQueue.Push(message);

    if (result == fw::ErrorCode::OK)
    {
      mPushFrameId++;
    }

    return result;
  }

  ImageMessage::Shared ImageQueue::Main(unsigned /*iTickNumber*/)
  {
    std::tuple<ImageMessage::Shared> framePool;
    if (mQueue.TryPop(framePool) != fw::ErrorCode::OK)
    {
      return nullptr;
    }

    ImageMessage::Shared image = std::get<0>(framePool);
    mLastFrameId = image->GetFrameId();
    mLastTimestamp = image->GetTimestamp();

    return image;
  }
}
