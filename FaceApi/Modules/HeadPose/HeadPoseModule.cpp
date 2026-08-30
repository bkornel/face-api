#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/HeadPose/HeadPoseModule.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

namespace face
{
  fw::ErrorCode HeadPoseModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);
    }

    return mDispatcher.Initialize(iSettings);
  }

  std::shared_ptr<PoseMessage> HeadPoseModule::Main(std::shared_ptr<ShapeMessage> iShapes)
  {
    DrainCommands();

    if (!iShapes || iShapes->IsEmpty()) return nullptr;

    FACE_PROFILER(3_Head_Pose);

    const cv::Mat cameraMatrix = fw::get_camera_matrix(iShapes->GetFrameSize());
    const auto& shapes = iShapes->GetShapes();

    PoseMessage::PoseVector poses(shapes.size());

    fw::parallel_for(
      shapes.size(),
      [&](std::size_t i) { mDispatcher.Estimate(shapes[i], cameraMatrix, poses[i]); },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return std::make_shared<PoseMessage>(std::move(poses), iShapes->GetFrameId(), iShapes->GetTimestamp());
  }
}
