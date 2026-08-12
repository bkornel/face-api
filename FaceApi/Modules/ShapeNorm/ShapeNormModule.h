#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceDataMessage.h"
#include "Modules/ShapeNorm/ShapeNormDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Adds the reference-aligned shapes to every face record. Works purely on the
  /// fitted shapes, so it needs no image.
  class ShapeNormModule : public fw::Module,
                          public fw::Port<std::shared_ptr<FaceDataMessage>(std::shared_ptr<FaceDataMessage>)>
  {
  public:

    ShapeNormModule() = default;

    virtual ~ShapeNormModule() = default;

    std::shared_ptr<FaceDataMessage> Main(std::shared_ptr<FaceDataMessage> iFaces) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeNormDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
