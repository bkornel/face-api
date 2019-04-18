#pragma once

#include "User/User.h"
#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "Modules/General/ModuleWithPort.hpp"
#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"
#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.h"

namespace face
{
  class UserProcessor :
    public ModuleWithPort<ActiveUsersMessage::Shared(ImageMessage::Shared, ActiveUsersMessage::Shared)>
  {
  public:
    UserProcessor() = default;

    virtual ~UserProcessor() = default;

    ActiveUsersMessage::Shared Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iActiveUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeModelDispatcher mShapeModelDispatcher;
    PoseEstimationDispatcher mPoseEstimationDispatcher;
    ShapeNormDispatcher mShapeNormDispatcher;
  };
}
