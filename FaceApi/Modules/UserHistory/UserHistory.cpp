#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/UserHistory/UserHistory.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <iomanip>

namespace face
{
  fw::ErrorCode UserHistory::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;
      if (fw::get_value(iSettings, "removeFreqMs", value))
        mRemoveFreqMs = fw::str::convert_to_number<int>(value);
    }

    mRemoveSW.Start();

    return fw::ErrorCode::OK;
  }

  std::shared_ptr<UserEntriesMessage> UserHistory::Main(std::shared_ptr<ActiveUsersMessage> iActiveUsers)
  {
    DrainCommands();

    if (!iActiveUsers || iActiveUsers->IsEmpty()) return nullptr;

    const fw::Timestamp currentTime = iActiveUsers->GetTimestamp();
    if (mRemoveSW.GetElapsedTimeMilliSec(false) > static_cast<double>(mRemoveFreqMs))
    {
      RemoveOldEntries(currentTime);
      mRemoveSW.Reset();
    }

    const auto& activeUsers = iActiveUsers->GetActiveUsers();

    for (auto& user : activeUsers)
    {
      // Push only the current entries
      const fw::Timestamp lastUpdateTs = user->GetLastUpdateTs();
      if (lastUpdateTs == currentTime)
      {
        // Snapshot, not the live User: a cast would alias the same mutating object.
        mEntryMap[user->GetUserId()].emplace_back(lastUpdateTs, std::make_shared<UserData>(*user));
      }
    }

    return std::make_shared<UserEntriesMessage>(mEntryMap, iActiveUsers->GetFrameId(), currentTime);
  }

  void UserHistory::RemoveOldEntries(fw::Timestamp iTimestamp)
  {
    const fw::Timestamp cutoff = iTimestamp - std::chrono::milliseconds(mRemoveFreqMs);

    {
      for (auto& h : mEntryMap)
      {
        auto& entries = h.second;
        const std::size_t sizeBefore = entries.size();

        entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const Entry& obj) {
                        return obj.first < cutoff;
                      }),
                      entries.end());

        const std::size_t count = sizeBefore - entries.size();
        if (count > 0U)
        {
          LOG(INFO) << "Number of entries deleted from User(" << h.first << "): " << count << " (" << cvRound((count * sizeof(UserData)) / 1024.0) << " KB).";
        }
      }

      std::erase_if(mEntryMap, [&](const EntryMap::value_type& obj) {
        return obj.second.empty();
      });
    }
  }

  void UserHistory::Clear()
  {
    mEntryMap.clear();
  }
}
