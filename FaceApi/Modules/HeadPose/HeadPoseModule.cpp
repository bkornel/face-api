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

  std::shared_ptr<FaceDataMessage> HeadPoseModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceDataMessage> iFaces)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iFaces || iFaces->IsEmpty()))
      return nullptr;

    FACE_PROFILER(3_Head_Pose);

    const cv::Mat cameraMatrix = fw::get_camera_matrix(iImage->GetSize());

    // A copy of the records: this module fills its own fields in its own message
    FaceDataMessage::FaceDataVector entries = iFaces->GetEntries();

    fw::parallel_for(
      entries.size(),
      [&](std::size_t i) { mDispatcher.Estimate(entries[i].data, cameraMatrix); },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return std::make_shared<FaceDataMessage>(std::move(entries), iFaces->GetFrameId(), iFaces->GetTimestamp());
  }
}
