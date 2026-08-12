#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ActiveUsersMessage.h"
#include "Modules/ShapeNorm/ShapeNormDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Aligns every user's shapes with the reference shape. Works purely on the fitted
  /// shapes, so it needs no image.
  class ShapeNormModule : public fw::Module,
                          public fw::Port<std::shared_ptr<ActiveUsersMessage>(std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    ShapeNormModule() = default;

    virtual ~ShapeNormModule() = default;

    std::shared_ptr<ActiveUsersMessage> Main(std::shared_ptr<ActiveUsersMessage> iUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeNormDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
