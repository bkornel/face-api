#include <opencv2/highgui/highgui.hpp>
#include <easyloggingpp/easyloggingpp.h>

#include "Modules/ImageQueue/ImageQueue.h"
#include "Messages/ImageSizeChangedMessage.h"

namespace face
{
	ImageQueue::ImageQueue() :
		fw::Module("ImageQueue"),
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

	fw::ErrorCode ImageQueue::Push(const cv::Mat& iFrame)
	{
		static unsigned sFrameId = 0U;

		if (iFrame.empty())
		{
			return fw::ErrorCode::BadParam;
		}

		commandHandler.Raise(std::make_shared<ImageSizeChangedMessage>(cv::Size(10, 10), 0U, 0LL));

		const fw::ErrorCode result = mQueue.Push(std::make_shared<ImageMessage>(iFrame, sFrameId, fw::get_current_time()));
		if (result == fw::ErrorCode::OK)
		{
			mImageSize = iFrame.size();
			sFrameId++;
		}

		return result;
	}

	ImageMessage::Shared ImageQueue::Pop(unsigned /*iTickNumber*/)
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