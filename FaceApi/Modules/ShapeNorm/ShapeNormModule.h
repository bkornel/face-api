#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/NormShapeMessage.h"
#include "Messages/PoseMessage.h"
#include "Messages/ShapeMessage.h"
#include "Modules/ShapeNorm/ShapeNormDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Aligns every fitted shape with the reference shape. The pose input is optional:
  /// without it only the 2-D half is normalized, since the 3-D shape comes from the pose.
  class ShapeNormModule : public fw::Module,
                          public fw::Port<std::shared_ptr<NormShapeMessage>(std::shared_ptr<ShapeMessage>, std::shared_ptr<PoseMessage>)>
  {
  public:

    ShapeNormModule() = default;

    virtual ~ShapeNormModule() = default;

    std::shared_ptr<NormShapeMessage> Main(std::shared_ptr<ShapeMessage> iShapes, std::shared_ptr<PoseMessage> iPoses) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeNormDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
