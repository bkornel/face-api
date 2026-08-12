#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ActiveUsersMessage.h"
#include "Messages/ImageMessage.h"
#include "Modules/HeadPose/PoseEstimationDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Estimates the head pose of every user whose shape was fitted this frame. The
  /// image is only needed for its size, which decides the camera matrix.
  class HeadPoseModule : public fw::Module,
                         public fw::Port<std::shared_ptr<ActiveUsersMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    HeadPoseModule() = default;

    virtual ~HeadPoseModule() = default;

    std::shared_ptr<ActiveUsersMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    PoseEstimationDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
