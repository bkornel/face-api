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

  std::shared_ptr<FaceDataMessage> ShapeNormModule::Main(std::shared_ptr<FaceDataMessage> iFaces)
  {
    DrainCommands();

    if (!iFaces || iFaces->IsEmpty()) return nullptr;

    FACE_PROFILER(3_Shape_Norm);

    // A copy of the records: this module fills its own fields in its own message
    FaceDataMessage::FaceDataVector entries = iFaces->GetEntries();

    fw::parallel_for(
      entries.size(),
      [&](std::size_t i) { mDispatcher.Normalize(entries[i].data); },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    return std::make_shared<FaceDataMessage>(std::move(entries), iFaces->GetFrameId(), iFaces->GetTimestamp());
  }
}
