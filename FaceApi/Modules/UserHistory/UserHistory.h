#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"
#include "User/User.h"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/UserEntriesMessage.h"

#include <map>

namespace face
{
  class UserHistory :
    public fw::Module,
    public fw::Port<UserEntriesMessage::Shared(ActiveUsersMessage::Shared)>
  {
    using Entry = UserEntriesMessage::Entry;
    using EntryMap = UserEntriesMessage::EntryMap;

  public:
    FW_DEFINE_SMART_POINTERS(UserHistory);

    UserHistory() = default;

    virtual ~UserHistory() = default;

    UserEntriesMessage::Shared Main(ActiveUsersMessage::Shared iActiveUsers) override;

    void Clear() override;

  private:
    using Entry = UserEntriesMessage::Entry;
    using EntryMap = UserEntriesMessage::EntryMap;

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void RemoveOldEntries(long long iTimestamp);

    EntryMap mEntryMap;
    fw::Stopwatch mRemoveSW;
    long long mRemoveFreqMs = 10000LL;
  };
}
