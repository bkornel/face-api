#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/FaceDetection/FaceDetection.h"

#include "Configuration.h"
#include "Framework/Profiler.h"
#include "Framework/Text.h"
#include "Messages/CommandMessage.h"
#include "Messages/ImageSizeChangedMessage.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <algorithm>

//#define CROP_FACE_RECT

namespace face
{
  const float FaceDetection::sForceDetectionSec = 0.5F;

  fw::ErrorCode FaceDetection::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "fileName", value))
        mCascadeFile = value;

      if (fw::get_value(iSettings, "imageScale", value))
      {
        mImageScaleFactor = fw::str::convert_to_number<float>(value);
        mImageScaleFactor = (std::max)((std::min)(mImageScaleFactor, 1.0F), 0.2F);
        mImageScaleFactorInv = (1.0F / mImageScaleFactor);
      }

      if (fw::get_value(iSettings, "detectionSec", value))
        mDetectionSec = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "detectionOverlap", value))
        mDetectionOverlap = fw::str::convert_to_number<float>(value);

      const cv::FileNode& dmsNode = iSettings["detectMultiScale"];
      if (!dmsNode.empty())
      {
        if (fw::get_value(dmsNode, "scaleFactor", value))
          mScaleFactor = fw::str::convert_to_number<float>(value);

        if (fw::get_value(dmsNode, "minNeighbors", value))
          mMinNeighbors = fw::str::convert_to_number<int>(value);

        if (fw::get_value(dmsNode, "minSize", value))
          mMinSizeFactor = fw::str::convert_to_number<float>(value);

        if (fw::get_value(dmsNode, "maxSize", value))
          mMaxSizeFactor = fw::str::convert_to_number<float>(value);

        if (fw::get_value(dmsNode, "flags", value))
          mFlags = fw::str::convert_to_number<int>(value);
      }
    }

    const auto& cascadePath = Configuration::GetInstance().GetDirectories().faceDetector + mCascadeFile;

    if (!mCascadeClassifier.load(cascadePath))
    {
      LOG(ERROR) << "Could not load cascade classifier: " << cascadePath;
      return fw::ErrorCode::NotFound;
    }

    mDetectionSW.Start();

    return fw::ErrorCode::OK;
  }

  void FaceDetection::HandleCommand(std::shared_ptr<fw::Message> iMessage)
  {
    Module::HandleCommand(iMessage);

    std::shared_ptr<CommandMessage> command = std::dynamic_pointer_cast<CommandMessage>(iMessage);
    if (command)
    {
      if (command->GetType() == CommandMessage::Type::RunFaceDetection && !mForceRun)
      {
        mForceRun = true;
        mDetectionSW.Reset();

        LOG(DEBUG) << "Force to run face detector.";
      }

      return;
    }

    std::shared_ptr<ImageSizeChangedMessage> imageSizeChanged = std::dynamic_pointer_cast<ImageSizeChangedMessage>(iMessage);
    if (imageSizeChanged)
    {
      if (mMinSizeFactor > 0.0F && mMinSizeFactor < 1.0F)
      {
        const int minSide = (std::min)(imageSizeChanged->GetWidth(), imageSizeChanged->GetHeight());
        mMinSize = { cvRound(minSide * mMinSizeFactor), cvRound(minSide * mMinSizeFactor) };
        LOG(DEBUG) << "Minimum face size of the detector: " << mMinSize;
      }

      if (mMaxSizeFactor > 0.0F && mMaxSizeFactor < 1.0F)
      {
        const int maxSide = (std::min)(imageSizeChanged->GetWidth(), imageSizeChanged->GetHeight());
        mMaxSize = { cvRound(maxSide * mMaxSizeFactor), cvRound(maxSide * mMaxSizeFactor) };
        LOG(DEBUG) << "Maximum face size of the detector: " << mMaxSize;
      }

      mForceRun = true;
      mDetectionSW.Reset();

      return;
    }
  }

  std::shared_ptr<RoiMessage> FaceDetection::Main(std::shared_ptr<ImageMessage> iImage)
  {
    DrainCommands();

    CV_DbgAssert(mDetectionSW.IsRunning());

    if (!iImage || iImage->IsEmpty() || !RunDetectection())
    {
      return nullptr;
    }

    FACE_PROFILER(1_Detect_Faces);

    const cv::Mat& image = iImage->GetFrameGray();
    cv::Mat resizedImage;

    // Resizing the image
    if (mImageScaleFactor < 1.0F)
    {
      cv::resize(image, resizedImage, {}, mImageScaleFactor, mImageScaleFactor);
      cv::equalizeHist(resizedImage, resizedImage);
    }
    else
    {
      cv::equalizeHist(image, resizedImage);
    }

    // Object detection
    std::vector<cv::Rect> faceROIs;
    mCascadeClassifier.detectMultiScale(resizedImage, faceROIs, mScaleFactor, mMinNeighbors, mFlags, mMinSize, mMaxSize);

    RemoveMultipleDetections(faceROIs);
    LOG(DEBUG) << "Number of detections: " << faceROIs.size();

    if (faceROIs.empty())
    {
      return nullptr;
    }

    // Every rectangle leaves this module clipped to the frame. Scaling a corner and a side
    // back up from the downscaled copy independently can put the far edge past it.
    const cv::Rect screenRect(0, 0, iImage->GetWidth(), iImage->GetHeight());

    std::vector<cv::Rect> scaledFaceROIs;
    for (auto& r : faceROIs)
    {
      cv::Rect rect(
        cvRound(r.x * mImageScaleFactorInv),
        cvRound(r.y * mImageScaleFactorInv),
        cvRound(r.width * mImageScaleFactorInv),
        cvRound(r.height * mImageScaleFactorInv)
      );

#ifdef CROP_FACE_RECT
      rect += cv::Point(cvRound(rect.width * 0.1F), 0);
      rect -= cv::Size(cvRound(rect.width * 0.2F), 0);
#endif

      rect &= screenRect;

      // A detection that survives the clip with no area left is not a face to report
      if (rect.area() <= 0) continue;

      scaledFaceROIs.emplace_back(rect);
    }

    if (scaledFaceROIs.empty())
    {
      return nullptr;
    }

    if (scaledFaceROIs.size() > 1)
    {
      std::sort(scaledFaceROIs.begin(), scaledFaceROIs.end(), [](const cv::Rect& lhs, const cv::Rect& rhs) {
        return lhs.area() > rhs.area();
      });
    }

    // Reset the timer
    mForceRun = false;
    mDetectionSW.Reset();

    return std::make_shared<RoiMessage>(scaledFaceROIs, mMinSize, mMaxSize, iImage->GetFrameId(), iImage->GetTimestamp());
  }

  void FaceDetection::RemoveMultipleDetections(std::vector<cv::Rect>& ioDetections)
  {
    for (auto fr1 = ioDetections.begin(); fr1 != ioDetections.end(); ++fr1)
    {
      for (auto fr2 = std::next(fr1); fr2 != ioDetections.end();)
      {
        if (fw::overlap_ratio(*fr1, *fr2) > mDetectionOverlap)
        {
          fr2 = ioDetections.erase(fr2);
        }
        else
        {
          fr2++;
        }
      }
    }
  }

  bool FaceDetection::RunDetectection() const
  {
    const double elapsedTimeSec = mDetectionSW.GetElapsedTimeSec(false);
    return (elapsedTimeSec > mDetectionSec) || (mForceRun && elapsedTimeSec > sForceDetectionSec);
  }
}
