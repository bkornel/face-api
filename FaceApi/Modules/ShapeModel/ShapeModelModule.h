#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceTrackMessage.h"
#include "Messages/ImageMessage.h"
#include "Messages/ShapeMessage.h"
#include "Modules/ShapeModel/IShapeFitter.h"

#include <memory>

namespace face
{
  /// @brief Fits the facial feature points of every tracked face on the frame. All it knows
  /// about a face is its rectangle and its id; what it returns is the shape that belongs to
  /// that rectangle. Faces whose fit failed are not forwarded.
  ///
  /// The method is chosen in the settings file: "3ddfa" (default) runs the 3DDFA_V2 network,
  /// which stays accurate when the head is turned; "clm" runs the classic constrained local
  /// model.
  class ShapeModelModule : public fw::Module,
                           public fw::Port<std::shared_ptr<ShapeMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<FaceTrackMessage>)>
  {
  public:

    ShapeModelModule() = default;

    virtual ~ShapeModelModule() = default;

    std::shared_ptr<ShapeMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks) override;

    void Clear() override
    {
      if (mFitter) mFitter->Clear();
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    std::unique_ptr<IShapeFitter> mFitter;
    bool mParallelUsers = true;
  };
}
