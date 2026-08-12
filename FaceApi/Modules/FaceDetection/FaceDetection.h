#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Stopwatch.h"
#include "Framework/Graph/Port.hpp"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"

#include <memory>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/face.hpp>

#include <string>

namespace face
{
  /// @brief Finds the faces on the frame with YuNet, a small convolutional detector that
  /// keeps working when the head is turned well away from the camera, where the cascade it
  /// replaced used to lose the face entirely.
  ///
  /// It does not run on every frame: the tracker follows the faces in between and asks for a
  /// detection when it wants one, which is what the command handling below is for.
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

    bool RunDetectection() const;

    cv::Ptr<cv::FaceDetectorYN> mDetector;
    fw::Stopwatch mDetectionSW;

    // General parameters
    std::string mModelFile = "face_detection_yunet_2023mar.onnx";
    float mImageScaleFactor = 1.0F;
    float mImageScaleFactorInv = 1.0F;
    float mDetectionSec = 10.0F;
    bool mForceRun = false;

    // Parameters of the detector
    float mScoreThreshold = 0.7F;
    float mNmsThreshold = 0.3F;
    int mTopK = 50;

    // The size range a face has to fall into, as a fraction of the shorter side
    cv::Size mMinSize;
    cv::Size mMaxSize;
    float mMinSizeFactor = 0.05F;
    float mMaxSizeFactor = 1.0F;

    // What the detector was last configured for; it needs the exact frame size
    cv::Size mDetectorSize;
  };
}
