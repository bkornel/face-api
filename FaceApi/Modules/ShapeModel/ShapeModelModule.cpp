#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeModel/ShapeModelModule.h"
#include "Modules/ShapeModel/ClmWrapper.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"
#include "Modules/ShapeModel/TddfaDispatcher.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <cstdint>

namespace face
{
  fw::ErrorCode ShapeModelModule::InitializeInternal(const cv::FileNode& iSettings)
  {
    std::string method = "3ddfa";
    std::string trackerFile = "face.tracker";
    std::string triFile = "face.tri";
    std::string conFile = "face.con";

    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "method", value))
        method = fw::str::to_lower(fw::str::trim(value));

      if (fw::get_value(iSettings, "trackerFile", value))
        trackerFile = value;

      if (fw::get_value(iSettings, "triFile", value))
        triFile = value;

      if (fw::get_value(iSettings, "conFile", value))
        conFile = value;

      if (fw::get_value(iSettings, "parallelUsers", value))
        mParallelUsers = fw::str::convert_to_boolean(value);
    }

    if (method == "clm")
      mFitter = std::make_unique<ShapeModelDispatcher>();
    else if (method == "3ddfa")
      mFitter = std::make_unique<TddfaDispatcher>();
    else
    {
      LOG(ERROR) << "Unknown shape model method: " << method;
      return fw::ErrorCode::BadParam;
    }

    LOG(INFO) << "Shape model method: " << method;

    fw::ErrorCode result = mFitter->Initialize(iSettings);
    if (result != fw::ErrorCode::OK) return result;

    // Loaded regardless of the method: the connection list drives the visualizer and the
    // reference shapes drive the normalization, whichever fitter produced the shape
    return ClmWrapper::GetInstance().Initialize(trackerFile, triFile, conFile);
  }

  std::shared_ptr<ShapeMessage> ShapeModelModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<FaceTrackMessage> iTracks)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iTracks || iTracks->IsEmpty()))
      return nullptr;

    FACE_PROFILER(2_Shape_Model);

    // Taken once, before the fan-out: the getter converts lazily behind a lock
    const cv::Mat frameGray = iImage->GetFrameGray();
    const cv::Mat frameBGR = iImage->GetFrameBGR();

    const auto& tracks = iTracks->GetTracks();

    // Per-track state is maintained on this thread; the fits below only use their own entry
    mFitter->BeginFrame(tracks);

    ShapeMessage::ShapeVector shapes(tracks.size());

    // Not vector<bool>: its packed bits would let neighbouring writes collide
    std::vector<uint8_t> fitted(tracks.size(), 0U);

    fw::parallel_for(
      tracks.size(),
      [&](std::size_t i) {
        fitted[i] = mFitter->Fit(tracks[i], frameGray, frameBGR, shapes[i]) ? 1U : 0U;
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
