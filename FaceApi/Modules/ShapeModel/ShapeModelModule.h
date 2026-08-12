#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceDataMessage.h"
#include "Messages/FaceTrackMessage.h"
#include "Messages/ImageMessage.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"

#include <memory>

namespace face
{
  /// @brief Fits the facial feature points of every tracked face on the frame and starts
  /// the per-face result record: the track plus its shape and refined rectangle. Faces
  /// whose fit failed are not forwarded, so nothing downstream sees an unfitted shape.
  class ShapeModelModule : public fw::Module,
                           public fw::Port<std::shared_ptr<FaceDataMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<FaceTrackMessage>)>
  {
  public:

    ShapeModelModule() = default;

    virtual ~ShapeModelModule() = default;

    std::shared_ptr<FaceDataMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks) override;

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
