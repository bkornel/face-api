#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"
#include "User/User.h"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/UserEntriesMessage.h"

#include <map>
#include <memory>

namespace face
{
  class UserHistory : public fw::Module,
                      public fw::Port<std::shared_ptr<UserEntriesMessage>(std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    UserHistory() = default;

    virtual ~UserHistory() = default;

    std::shared_ptr<UserEntriesMessage> Main(std::shared_ptr<ActiveUsersMessage> iActiveUsers) override;

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
