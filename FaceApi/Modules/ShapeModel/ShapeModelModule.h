#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ShapeMessage.h"
#include "Messages/FaceTrackMessage.h"
#include "Messages/ImageMessage.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Fits the facial feature points of every tracked face on the frame. All it knows
  /// about a face is its rectangle and its id; what it returns is the shape that belongs to
  /// that rectangle. Faces whose fit failed are not forwarded.
  class ShapeModelModule : public fw::Module,
                           public fw::Port<std::shared_ptr<ShapeMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<FaceTrackMessage>)>
  {
  public:

    ShapeModelModule() = default;

    virtual ~ShapeModelModule() = default;

    std::shared_ptr<ShapeMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks) override;

    void Clear() override
    {
      mDispatcher.Clear();
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    ShapeModelDispatcher mDispatcher;
    bool mParallelUsers = true;
  };
}
