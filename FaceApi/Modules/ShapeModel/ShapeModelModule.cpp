#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeModel/ShapeModelModule.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <cstdint>

namespace face
{
  fw::ErrorCode ShapeModelModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);
    }

    return mDispatcher.Initialize(iSettings);
  }

  std::shared_ptr<ShapeMessage> ShapeModelModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iTracks || iTracks->IsEmpty()))
      return nullptr;

    FACE_PROFILER(2_Shape_Model);

    // Taken once, before the fan-out: the getter converts lazily behind a lock
    const cv::Mat frameBGR = iImage->GetFrameBGR();

    const auto& tracks = iTracks->GetTracks();

    // Per-track state is maintained on this thread; the fits below only use their own entry
    mDispatcher.BeginFrame(tracks);

    ShapeMessage::ShapeVector shapes(tracks.size());

    // Not vector<bool>: its packed bits would let neighbouring writes collide
    std::vector<uint8_t> fitted(tracks.size(), 0U);

    fw::parallel_for(
      tracks.size(),
      [&](std::size_t i) {
        fitted[i] = mDispatcher.Fit(tracks[i], frameBGR, shapes[i]) ? 1U : 0U;
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    ShapeMessage::ShapeVector fittedShapes;
    fittedShapes.reserve(shapes.size());

    for (std::size_t i = 0U; i < shapes.size(); ++i)
    {
      if (fitted[i]) fittedShapes.emplace_back(std::move(shapes[i]));
    }

    if (fittedShapes.empty()) return nullptr;

    return std::make_shared<ShapeMessage>(std::move(fittedShapes), iImage->GetSize(), iTracks->GetFrameId(), iTracks->GetTimestamp());
  }
}
