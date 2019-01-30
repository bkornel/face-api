#include <opencv2/imgproc/imgproc.hpp>

#include "Messages/ImageMessage.h"

namespace face
{
	std::recursive_mutex ImageMessage::sMutex;

	ImageMessage::ImageMessage(const cv::Mat& iImage, unsigned iFrameId, long long iTimestamp) :
		Message(iFrameId, iTimestamp)
	{
		assert(!iImage.empty());
		mFrames.first = iImage.clone();
	}

	const cv::Mat& ImageMessage::GetFrameGray()
	{
		assert(!IsEmpty());
		std::lock_guard<std::recursive_mutex> lock(sMutex);

		if (mFrames.second.empty())
		{
			mFrames.second.create(GetSize(), CV_8UC1);
			cv::cvtColor(GetFrameBGR(), mFrames.second, cv::COLOR_BGR2GRAY);
		}

		return mFrames.second;
	}

	const cv::Mat& ImageMessage::GetResizedBGR(float iScaleFactor)
	{
		assert(!IsEmpty() && iScaleFactor > 0.0F);

		if (std::abs(iScaleFactor - 1.0F) <= std::numeric_limits<float>::epsilon())
		{
			return GetFrameBGR();
		}

		std::lock_guard<std::recursive_mutex> lock(sMutex);

		const int width = cvRound(GetWidth() * iScaleFactor);
		cv::Mat& resized = mResizedFrames[width].first;

		if (resized.empty())
		{
			cv::resize(GetFrameBGR(), resized, {}, iScaleFactor, iScaleFactor);
		}

		return resized;
	}

	const cv::Mat& ImageMessage::GetResizedGray(float iScaleFactor)
	{
		assert(!IsEmpty() && iScaleFactor > 0.0F);

		if (std::abs(iScaleFactor - 1.0F) <= std::numeric_limits<float>::epsilon())
		{
			return GetFrameGray();
		}

		std::lock_guard<std::recursive_mutex> lock(sMutex);

		const int width = cvRound(GetWidth() * iScaleFactor);
		cv::Mat& resized = mResizedFrames[width].second;

		if (resized.empty())
		{
			cv::resize(GetFrameGray(), resized, {}, iScaleFactor, iScaleFactor);
		}

		return resized;
	}
}