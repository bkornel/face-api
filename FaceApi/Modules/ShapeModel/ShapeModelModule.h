#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ActiveUsersMessage.h"
#include "Messages/ImageMessage.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Fits the facial feature points of every user on the frame. Only the users whose
  /// fit succeeded are forwarded, so downstream modules never see an unfitted shape.
  class ShapeModelModule : public fw::Module,
                           public fw::Port<std::shared_ptr<ActiveUsersMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    ShapeModelModule() = default;

    virtual ~ShapeModelModule() = default;

    std::shared_ptr<ActiveUsersMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeModelDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
