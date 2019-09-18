#pragma once

#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"

#include "User/User.h"
#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"
#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.h"

namespace face
{
  class UserProcessor :
    public fw::Module,
    public fw::Port<ActiveUsersMessage::Shared(ImageMessage::Shared, ActiveUsersMessage::Shared)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(UserProcessor);

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
