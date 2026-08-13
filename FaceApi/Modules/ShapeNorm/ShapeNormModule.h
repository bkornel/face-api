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
  /// @brief Aligns every fitted shape with the reference shape, in two and in three
  /// dimensions.
  ///
  /// The pose input is no longer read. It was there because the 3-D shape used to come from
  /// the pose module, which produces the canonical model moved to where the head is - a
  /// shape that normalises to the same answer on every frame of every face. The fitter's own
  /// 3-D shape is the one with a face in it, and it arrives on the first port. The port is
  /// kept so that a settings file wiring it does not become invalid.
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
