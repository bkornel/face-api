#include "Messages/ImageMessage.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace face
{
  // Clones, because the caller usually owns a buffer it keeps writing to - a camera frame
  ImageMessage::ImageMessage(const cv::Mat& iImage, unsigned iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp)
  {
    CV_DbgAssert(!iImage.empty());
    mFrames.first = iImage.clone();
  }

  // Takes the buffer over, for callers that produced it and have no further use for it.
  // Saves a full frame copy per frame, which is the pipeline's largest single memcpy.
  ImageMessage::ImageMessage(cv::Mat&& iImage, unsigned iFrameId, fw::Timestamp iTimestamp) :
    Message(iFrameId, iTimestamp)
  {
    CV_DbgAssert(!iImage.empty());
    mFrames.first = std::move(iImage);
  }

  // The cached matrices are returned by value: a cv::Mat copy only bumps a refcount, and
  // handing out a reference to a member would take the caller past the lock protecting it.
  cv::Mat ImageMessage::GetFrameGray()
  {
    CV_DbgAssert(!IsEmpty());
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (mFrames.second.empty())
    {
      mFrames.second.create(GetSize(), CV_8UC1);
      cv::cvtColor(GetFrameBGR(), mFrames.second, cv::COLOR_BGR2GRAY);
    }

    return mFrames.second;
  }

  cv::Mat ImageMessage::GetResizedBGR(float iScaleFactor)
  {
    CV_DbgAssert(!IsEmpty() && iScaleFactor > 0.0F);

    if (std::abs(iScaleFactor - 1.0F) <= std::numeric_limits<float>::epsilon())
    {
      return GetFrameBGR();
    }

    std::lock_guard<std::recursive_mutex> lock(mMutex);

    const int width = cvRound(GetWidth() * iScaleFactor);
    cv::Mat& resized = mResizedFrames[width].first;

    if (resized.empty())
    {
      cv::resize(GetFrameBGR(), resized, {}, iScaleFactor, iScaleFactor);
    }

    return resized;
  }

  cv::Mat ImageMessage::GetResizedGray(float iScaleFactor)
  {
    CV_DbgAssert(!IsEmpty() && iScaleFactor > 0.0F);

    if (std::abs(iScaleFactor - 1.0F) <= std::numeric_limits<float>::epsilon())
    {
      return GetFrameGray();
    }

    std::lock_guard<std::recursive_mutex> lock(mMutex);

    const int width = cvRound(GetWidth() * iScaleFactor);
    cv::Mat& resized = mResizedFrames[width].second;

    if (resized.empty())
    {
      cv::resize(GetFrameGray(), resized, {}, iScaleFactor, iScaleFactor);
    }

    return resized;
  }
}
