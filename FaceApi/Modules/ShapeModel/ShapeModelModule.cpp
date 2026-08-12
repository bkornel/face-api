#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/Parallel.h"
#include "Modules/ShapeModel/ShapeModelModule.h"
#include "Modules/ShapeModel/ClmWrapper.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

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

  std::shared_ptr<ActiveUsersMessage> ShapeModelModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iUsers || iUsers->IsEmpty()))
      return nullptr;

    FACE_PROFILER(2_Shape_Model);

    // Taken once, before the fan-out: the getter converts lazily behind a lock
    const cv::Mat frame = iImage->GetFrameGray();

    const auto& users = iUsers->GetActiveUsers();

    // The model map is maintained on this thread; the fits below only read it
    mDispatcher.RetainModels(users);

    struct Job
    {
      std::shared_ptr<User> user;
      std::shared_ptr<ShapeModel> model;
    };

    std::vector<Job> jobs;
    jobs.reserve(users.size());

    for (const auto& user : users)
    {
      if (user) jobs.emplace_back(Job{ user, mDispatcher.GetModel(*user) });
    }

    // Each user carries its own model, so the fits are independent of each other. Not
    // vector<bool>: its packed bits would let neighbouring writes collide.
    std::vector<uint8_t> fitted(jobs.size(), 0U);

    fw::parallel_for(
      jobs.size(),
      [&](std::size_t i) { fitted[i] = mDispatcher.Fit(*jobs[i].user, *jobs[i].model, frame) ? 1U : 0U; },
      mParallelUsers ? fw::get_thread_pool_executor() : nullptr);

    ActiveUsersMessage::UserVector fittedUsers;
    fittedUsers.reserve(jobs.size());

    for (std::size_t i = 0U; i < jobs.size(); ++i)
    {
      if (fitted[i]) fittedUsers.emplace_back(jobs[i].user);
    }

    if (fittedUsers.empty()) return nullptr;

    return std::make_shared<ActiveUsersMessage>(fittedUsers, iUsers->GetFrameId(), iUsers->GetTimestamp());
  }
}
