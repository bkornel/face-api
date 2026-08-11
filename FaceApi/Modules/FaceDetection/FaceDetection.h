#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Stopwatch.h"
#include "Framework/Graph/Port.hpp"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"

#include <memory>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <string>

namespace face
{
  class FaceDetection : public fw::Module,
                        public fw::Port<std::shared_ptr<RoiMessage>(std::shared_ptr<ImageMessage>)>
  {
  public:

    FaceDetection() = default;

    virtual ~FaceDetection() = default;

    std::shared_ptr<RoiMessage> Main(std::shared_ptr<ImageMessage> iImage) override;

  protected:
    const static float sForceDetectionSec;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void HandleCommand(std::shared_ptr<fw::Message> iMessage) override;

    void RemoveMultipleDetections(std::vector<cv::Rect>& ioDetections);

    bool RunDetectection() const;

    cv::CascadeClassifier mCascadeClassifier; ///< The OpenCV cascade classifier
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
