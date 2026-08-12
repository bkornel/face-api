#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeNorm/ShapeNormModule.h"
#include "Modules/ShapeModel/ClmWrapper.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

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

  std::shared_ptr<ActiveUsersMessage> ShapeNormModule::Main(std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if (!iUsers || iUsers->IsEmpty()) return nullptr;

    FACE_PROFILER(3_Shape_Norm);

    const auto& users = iUsers->GetActiveUsers();

    fw::parallel_for(
      users.size(),
      [&](std::size_t i) {
        if (users[i]) mDispatcher.Normalize(*users[i]);
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return iUsers;
  }
}
