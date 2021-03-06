#pragma once

#include "Framework/Module.h"
#include "Framework/Stopwatch.h"
#include "Framework/Port.hpp"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <string>

namespace face
{
  class FaceDetection :
    public fw::Module,
    public fw::Port<RoiMessage::Shared(ImageMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(FaceDetection);

    FaceDetection() = default;

    virtual ~FaceDetection() = default;

    RoiMessage::Shared Main(ImageMessage::Shared iImage) override;

  protected:
    const static float sForceDetectionSec;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void OnCommand(fw::Message::Shared iMessage) override;

    void RemoveMultipleDetections(std::vector<cv::Rect>& ioDetections);

    bool RunDetectection() const;

    cv::CascadeClassifier mCascadeClassifier;   ///< The OpenCV cascade classifier
    fw::Stopwatch mDetectionSW;

    // General parameters
    std::string mCascadeFile = "haarcascade_frontalface_alt2.xml";
    float mImageScaleFactor = 1.0F;
    float mImageScaleFactorInv = 1.0F;
    float mDetectionOverlap = 0.2F;
    float mDetectionSec = 10.0F;
    bool mForceRun = false;

    // Parameter of detectMultiScale(...)
    cv::Size mMinSize;
    cv::Size mMaxSize;
    float mScaleFactor = 1.1F;
    int mMinNeighbors = 3;
    float mMinSizeFactor = 0.05f;
    float mMaxSizeFactor = 1.0F;
    int mFlags = 0;
  };
}
