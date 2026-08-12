#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeModel/ShapeModelModule.h"
#include "Modules/ShapeModel/ClmWrapper.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <cstdint>

namespace face
{
  fw::ErrorCode ShapeModelModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    std::string trackerFile = "face.tracker";
    std::string triFile = "face.tri";
    std::string conFile = "face.con";

    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "trackerFile", value))
        trackerFile = value;

      if (fw::get_value(iSettings, "triFile", value))
        triFile = value;

      if (fw::get_value(iSettings, "conFile", value))
        conFile = value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);

      mDispatcher.Initialize(iSettings);
    }

    return ClmWrapper::GetInstance().Initialize(trackerFile, triFile, conFile);
  }

  std::shared_ptr<FaceDataMessage> ShapeModelModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iTracks || iTracks->IsEmpty()))
      return nullptr;

    FACE_PROFILER(2_Shape_Model);

    // Taken once, before the fan-out: the getter converts lazily behind a lock
    const cv::Mat frame = iImage->GetFrameGray();

    const auto& tracks = iTracks->GetTracks();

    // The model map is maintained on this thread; the fits below only use their own entry
    mDispatcher.RetainModels(tracks);

    struct Job
    {
      const TrackedFace* track = nullptr;
      ShapeModelDispatcher::TrackModel* model = nullptr;
    };

    std::vector<Job> jobs;
    jobs.reserve(tracks.size());

    for (const auto& track : tracks)
      jobs.emplace_back(Job{ &track, &mDispatcher.GetModel(track) });

    // Every job owns its model and writes its own entry, so the fits are independent
    FaceDataMessage::FaceDataVector entries(jobs.size());

    // Not vector<bool>: its packed bits would let neighbouring writes collide
    std::vector<uint8_t> fitted(jobs.size(), 0U);

    fw::parallel_for(
      jobs.size(),
      [&](std::size_t i) {
        entries[i].track = *jobs[i].track;
        fitted[i] = mDispatcher.Fit(*jobs[i].track, *jobs[i].model, frame, entries[i].data) ? 1U : 0U;

        // The record carries the refined rectangle from here on
        if (fitted[i]) entries[i].track.faceRect = entries[i].data.GetFaceRect();
      },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    FaceDataMessage::FaceDataVector fittedEntries;
    fittedEntries.reserve(entries.size());

    for (std::size_t i = 0U; i < entries.size(); ++i)
    {
      if (fitted[i]) fittedEntries.emplace_back(std::move(entries[i]));
    }

    if (fittedEntries.empty()) return nullptr;

    return std::make_shared<FaceDataMessage>(std::move(fittedEntries), iTracks->GetFrameId(), iTracks->GetTimestamp());
  }
}
