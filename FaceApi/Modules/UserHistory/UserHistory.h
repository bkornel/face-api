#pragma once

#include "Framework/Stopwatch.h"
#include "User/User.h"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/UserEntriesMessage.h"
#include "Modules/General/ModuleWithPort.hpp"

#include <map>

namespace face
{
	class UserHistory :
		public ModuleWithPort<UserEntriesMessage::Shared(ActiveUsersMessage::Shared)>
	{
		using Entry = UserEntriesMessage::Entry;
		using EntryMap = UserEntriesMessage::EntryMap;

	public:
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
