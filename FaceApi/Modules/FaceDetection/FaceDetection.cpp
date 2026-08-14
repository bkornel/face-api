#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/DnnOptions.h"
#include "Framework/ErrorCode.h"
#include "Modules/FaceDetection/FaceDetection.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <algorithm>

namespace face
{
  const float FaceDetection::sForceDetectionSec = 0.5F;

  fw::ErrorCode FaceDetection::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "fileName", value))
        mModelFile = value;

      if (fw::get_value(iSettings, "imageScale", value))
      {
        mImageScaleFactor = std::clamp(fw::str::convert_to_number<float>(value), 0.2F, 1.0F);
        mImageScaleFactorInv = (1.0F / mImageScaleFactor);
      }

      if (fw::get_value(iSettings, "detectionSec", value))
        mDetectionSec = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "scoreThreshold", value))
        mScoreThreshold = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "nmsThreshold", value))
        mNmsThreshold = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "topK", value))
        mTopK = fw::str::convert_to_number<int>(value);

      if (fw::get_value(iSettings, "minSize", value))
        mMinSizeFactor = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "maxSize", value))
        mMaxSizeFactor = fw::str::convert_to_number<float>(value);
    }

    // Relative to the working directory, like every path a settings file carries
    const std::string modelPath = GetWorkingDirectory() + mModelFile;

    const fw::DnnOptions dnn = fw::get_dnn_options(iSettings);

    try
    {
      // The input size is set for real once a frame arrives and its size is known
      mDetector = cv::FaceDetectorYN::create(modelPath, "", { 320, 320 }, mScoreThreshold, mNmsThreshold,
                                             mTopK, dnn.backend, dnn.target);
    }
    catch (const cv::Exception& iException)
    {
      LOG(ERROR) << "Could not load the face detector from " << modelPath << ": " << iException.what();
      return fw::ErrorCode::NotFound;
    }

    if (!mDetector)
    {
      LOG(ERROR) << "Could not create the face detector from " << modelPath;
      return fw::ErrorCode::NotFound;
    }

    mDetectionSW.Start();

    return fw::ErrorCode::OK;
  }

  void FaceDetection::ForceDetection()
  {
    if (mForceRun) return;

    mForceRun = true;
    mDetectionSW.Reset();

    LOG(DEBUG) << "Force to run face detector.";
  }

  void FaceDetection::OnImageSizeChanged(const cv::Size& iSize)
  {
    const int shorterSide = (std::min)(iSize.width, iSize.height);

    if (mMinSizeFactor > 0.0F && mMinSizeFactor < 1.0F)
    {
      mMinSize = { cvRound(shorterSide * mMinSizeFactor), cvRound(shorterSide * mMinSizeFactor) };
      LOG(DEBUG) << "Minimum face size of the detector: " << mMinSize;
    }

    if (mMaxSizeFactor > 0.0F && mMaxSizeFactor < 1.0F)
    {
      mMaxSize = { cvRound(shorterSide * mMaxSizeFactor), cvRound(shorterSide * mMaxSizeFactor) };
      LOG(DEBUG) << "Maximum face size of the detector: " << mMaxSize;
    }

    mForceRun = true;
    mDetectionSW.Reset();
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

    // The detector wants colour, and it wants to be told the exact size it will be given
    const cv::Mat image = (mImageScaleFactor < 1.0F)
                            ? iImage->GetResizedBGR(mImageScaleFactor)
                            : iImage->GetFrameBGR();

    if (image.size() != mDetectorSize)
    {
      mDetector->setInputSize(image.size());
      mDetectorSize = image.size();
    }

    // Rows of [x, y, w, h, five landmarks, score]; the detector runs its own suppression
    cv::Mat detections;
    mDetector->detect(image, detections);

    LOG(DEBUG) << "Number of detections: " << detections.rows;

    if (detections.empty())
    {
      return nullptr;
    }

    // Every rectangle leaves this module clipped to the frame. Scaling a corner and a side
    // back up from the downscaled copy independently can put the far edge past it.
    const cv::Rect screenRect(0, 0, iImage->GetWidth(), iImage->GetHeight());

    std::vector<cv::Rect> faceROIs;
    faceROIs.reserve(detections.rows);

    for (int i = 0; i < detections.rows; ++i)
    {
      const float* row = detections.ptr<float>(i);

      cv::Rect rect(
        cvRound(row[0] * mImageScaleFactorInv),
        cvRound(row[1] * mImageScaleFactorInv),
        cvRound(row[2] * mImageScaleFactorInv),
        cvRound(row[3] * mImageScaleFactorInv)
      );

      rect &= screenRect;

      // A detection that survives the clip with no area left is not a face to report
      if (rect.area() <= 0) continue;

      // The size range the tracker also holds faces to, applied here so a face outside it
      // never starts a track in the first place
      if (!mMinSize.empty() && (rect.width < mMinSize.width || rect.height < mMinSize.height)) continue;
      if (!mMaxSize.empty() && (rect.width > mMaxSize.width || rect.height > mMaxSize.height)) continue;

      faceROIs.emplace_back(rect);
    }

    if (faceROIs.empty())
    {
      return nullptr;
    }

    // The detector returns its rows by descending score; the tracker takes the first ones
    // when it has fewer slots than there are faces, so put the biggest first
    if (faceROIs.size() > 1)
    {
      std::sort(faceROIs.begin(), faceROIs.end(), [](const cv::Rect& lhs, const cv::Rect& rhs) {
        return lhs.area() > rhs.area();
      });
    }

    // Reset the timer
    mForceRun = false;
    mDetectionSW.Reset();

    return std::make_shared<RoiMessage>(faceROIs, mMinSize, mMaxSize, iImage->GetFrameId(), iImage->GetTimestamp());
  }

  bool FaceDetection::RunDetectection() const
  {
    const double elapsedTimeSec = mDetectionSW.GetElapsedTimeSec(false);
    return (elapsedTimeSec > mDetectionSec) || (mForceRun && elapsedTimeSec > sForceDetectionSec);
  }
}
