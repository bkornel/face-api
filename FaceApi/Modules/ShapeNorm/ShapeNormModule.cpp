#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeNorm/ShapeNormModule.h"
#include "Modules/ShapeModel/ClmWrapper.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <map>

namespace face
{
  fw::ErrorCode ShapeNormModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    // The reference shapes come from the tracker files, which the shape model module loads.
    // Normalizing without a fitted shape makes no sense anyway, so requiring it is honest.
    if (!ClmWrapper::GetInstance().IsInitialized())
    {
      LOG(ERROR) << "The shape model module has to be configured before shape normalization.";
      return fw::ErrorCode::BadState;
    }

    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);
    }

    return mDispatcher.Initialize(iSettings);
  }

  std::shared_ptr<NormShapeMessage> ShapeNormModule::Main(std::shared_ptr<ShapeMessage> iShapes, std::shared_ptr<PoseMessage> iPoses)
  {
    DrainCommands();

    if (!iShapes || iShapes->IsEmpty()) return nullptr;

    FACE_PROFILER(3_Shape_Norm);

    const auto& shapes = iShapes->GetShapes();

    // The pose input is optional; where a pose exists its 3-D shape is normalized as well
    std::map<int, const PoseDescriptor*> poseById;
    if (iPoses)
    {
      for (const auto& pose : iPoses->GetPoses())
        poseById.emplace(pose.trackId, &pose);
    }

    NormShapeMessage::NormShapeVector normShapes(shapes.size());

    fw::parallel_for(
      shapes.size(),
      [&](std::size_t i) {
        normShapes[i].trackId = shapes[i].trackId;
        normShapes[i].normShape2D = mDispatcher.Normalize2D(shapes[i].shape2D);

        auto pose = poseById.find(shapes[i].trackId);
        if (pose != poseById.end())
          normShapes[i].normShape3D = mDispatcher.Normalize3D(pose->second->shape3D);
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return std::make_shared<NormShapeMessage>(std::move(normShapes), iShapes->GetFrameId(), iShapes->GetTimestamp());
  }
}
