#pragma once

#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"

#include "User/User.h"
#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"
#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.h"
#include <memory>

namespace face
{
  class UserProcessor : public fw::Module,
                        public fw::Port<std::shared_ptr<ActiveUsersMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    UserProcessor() = default;

    virtual ~UserProcessor() = default;

    std::shared_ptr<ActiveUsersMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iActiveUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeModelDispatcher mShapeModelDispatcher;
    PoseEstimationDispatcher mPoseEstimationDispatcher;
    ShapeNormDispatcher mShapeNormDispatcher;
  };
}
