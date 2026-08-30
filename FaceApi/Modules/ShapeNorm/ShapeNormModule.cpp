#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeNorm/ShapeNormModule.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

namespace face
{
  fw::ErrorCode ShapeNormModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);
    }

    return mDispatcher.Initialize(iSettings);
  }

  std::shared_ptr<NormShapeMessage> ShapeNormModule::Main(std::shared_ptr<ShapeMessage> iShapes,
                                                          std::shared_ptr<PoseMessage> /*iPoses*/)
  {
    DrainCommands();

    if (!iShapes || iShapes->IsEmpty()) return nullptr;

    FACE_PROFILER(3_Shape_Norm);

    const auto& shapes = iShapes->GetShapes();

    NormShapeMessage::NormShapeVector normShapes(shapes.size());

    fw::parallel_for(
      shapes.size(),
      [&](std::size_t i) {
        normShapes[i].trackId = shapes[i].trackId;
        normShapes[i].normShape2D = mDispatcher.Normalize2D(shapes[i].shape2D);

        // The fitted shape, not the pose module's: that one is the canonical model moved to
        // where the head is, so normalising it gives the same answer on every frame of every
        // face. This one is the face the network actually reconstructed.
        if (!shapes[i].shape3D.empty())
          normShapes[i].normShape3D = mDispatcher.Normalize3D(shapes[i].shape3D);
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return std::make_shared<NormShapeMessage>(std::move(normShapes), iShapes->GetFrameId(), iShapes->GetTimestamp());
  }
}
