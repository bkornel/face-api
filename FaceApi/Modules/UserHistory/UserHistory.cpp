#include "Modules/UserHistory/UserHistory.h"

#include "Framework/Profiler.h"
#include "Framework/UtilOCV.h"
#include "Framework/UtilString.h"

#include <easyloggingpp/easyloggingpp.h>
#include <iomanip>

namespace face
{
  fw::ErrorCode UserHistory::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;
      if (fw::ocv::get_value(iSettings, "removeFreqMs", value))
        mRemoveFreqMs = fw::str::convert_to_number<int>(value);
    }

    mRemoveSW.Start();

    return fw::ErrorCode::OK;
  }

  UserEntriesMessage::Shared UserHistory::Main(ActiveUsersMessage::Shared iActiveUsers)
  {
    if (!iActiveUsers || iActiveUsers->IsEmpty()) return nullptr;

    const long long currentTime = iActiveUsers->GetTimestamp();
    if (mRemoveSW.GetElapsedTimeMilliSec(false) > mRemoveFreqMs)
    {
      RemoveOldEntries(currentTime);
      mRemoveSW.Reset();
    }

    const auto& activeUsers = iActiveUsers->GetActiveUsers();

    for (auto& user : activeUsers)
    {
      // Push only the current entries
      const long long lastUpdateTs = user->GetLastUpdateTs();
      if (lastUpdateTs == currentTime)
      {
        UserData::Shared userData = std::dynamic_pointer_cast<UserData>(user);
        if (userData != nullptr)
        {
          mEntryMap[user->GetUserId()].push_back(std::make_pair(lastUpdateTs, userData));
        }
      }
    }

    return std::make_shared<UserEntriesMessage>(mEntryMap, iActiveUsers->GetFrameId(), currentTime);
  }

  void UserHistory::RemoveOldEntries(long long iTimestamp)
  {
    const long long diff = iTimestamp - mRemoveFreqMs;

    if (diff > 0LL)
    {
      for (auto& h : mEntryMap)
      {
        auto& entries = h.second;
        const std::size_t sizeBefore = entries.size();

        entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const Entry& obj)
        {
          return obj.first < diff;
        }),
          entries.end()
          );

        const std::size_t count = sizeBefore - entries.size();
        if (count > 0U)
        {
          LOG(INFO) << "Number of entries deleted from User(" << h.first << "): " <<
            count << " (" << cvRound((count * sizeof(UserData)) / 1024.0) << " KB).";
        }
      }

      fw::remove_if(mEntryMap, [&](const EntryMap::value_type& obj)
      {
        return obj.second.empty();
      });
    }
  }

  void UserHistory::Clear()
  {
    mEntryMap.clear();
  }
}
