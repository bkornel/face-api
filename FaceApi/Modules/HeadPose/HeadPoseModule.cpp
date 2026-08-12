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

  std::shared_ptr<ActiveUsersMessage> HeadPoseModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iUsers || iUsers->IsEmpty()))
      return nullptr;

    FACE_PROFILER(3_Head_Pose);

    const cv::Mat cameraMatrix = fw::get_camera_matrix(iImage->GetSize());
    const auto& users = iUsers->GetActiveUsers();

    fw::parallel_for(
      users.size(),
      [&](std::size_t i) {
        if (users[i]) mDispatcher.Estimate(*users[i], cameraMatrix);
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    // Every user that arrived leaves with a pose, the message travels on unchanged
    return iUsers;
  }
}
