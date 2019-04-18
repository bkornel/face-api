#pragma once

#include "Framework/Message.h"
#include "User/User.h"

namespace face
{
  class UserEntriesMessage :
    public fw::Message
  {
  public:
    FW_DEFINE_SMART_POINTERS(UserEntriesMessage);

    using Entry = std::pair<long long, UserData::Shared>;
    using EntryMap = std::map<int, std::vector<Entry>>;

    UserEntriesMessage(const EntryMap& iEntryMap, unsigned iFrameId, long long iTimestamp);

    virtual ~UserEntriesMessage() = default;

    friend inline std::ostream& operator<<(std::ostream& ioStream, const UserEntriesMessage& iMessage);

    inline bool IsEmpty() const
    {
      return mEntryMap.empty();
    }

    inline std::size_t GetSize() const
    {
      return mEntryMap.size();
    }

    inline std::size_t GetNumerOfEntries() const
    {
      std::size_t count = 0U;
      for (auto& e : mEntryMap)
        count += e.second.size();

      return count;
    }

  private:
    EntryMap mEntryMap;
  };

  inline std::ostream& operator<< (std::ostream& ioStream, const UserEntriesMessage& iMessage)
  {
    const fw::Message& base(iMessage);
    ioStream << base << ", [Derived] Number of users: " << iMessage.GetSize() <<
      ", numer of entries: " << iMessage.GetNumerOfEntries();
    return ioStream;
  }
}
