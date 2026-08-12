#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceDataMessage.h"
#include "Messages/ImageMessage.h"
#include "Modules/HeadPose/PoseEstimationDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Adds the head pose to every fitted face record. The image is only needed for
  /// its size, which decides the camera matrix.
  class HeadPoseModule : public fw::Module,
                         public fw::Port<std::shared_ptr<FaceDataMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<FaceDataMessage>)>
  {
  public:

    HeadPoseModule() = default;

    virtual ~HeadPoseModule() = default;

    std::shared_ptr<FaceDataMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceDataMessage> iFaces) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    PoseEstimationDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
