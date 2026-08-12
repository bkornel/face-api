#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/PoseMessage.h"
#include "Messages/ShapeMessage.h"
#include "Modules/HeadPose/PoseEstimationDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Estimates the 6DoF head pose belonging to every fitted shape. All it needs is
  /// the shapes: the camera matrix comes from the frame size the shape message carries.
  class HeadPoseModule : public fw::Module,
                         public fw::Port<std::shared_ptr<PoseMessage>(std::shared_ptr<ShapeMessage>)>
  {
  public:

    HeadPoseModule() = default;

    virtual ~HeadPoseModule() = default;

    std::shared_ptr<PoseMessage> Main(std::shared_ptr<ShapeMessage> iShapes) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    PoseEstimationDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
