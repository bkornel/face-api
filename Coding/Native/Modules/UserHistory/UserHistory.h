#pragma once

#include <map>

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

#include "User/User.h"
#include "Messages/ActiveUsersMessage.h"
#include "Messages/UserEntriesMessage.h"

namespace face
{
	class UserHistory :
		public fw::Module,
		public fw::Port<UserEntriesMessage::Shared>
	{
		using Entry = UserEntriesMessage::Entry;
		using EntryMap = UserEntriesMessage::EntryMap;

	public:
		UserHistory();

		virtual ~UserHistory() = default;

		UserEntriesMessage::Shared Process(ActiveUsersMessage::Shared iActiveUsers);

		void Clear() override;

	private:
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		void RemoveOldEntries(long long iTimestamp);

		EntryMap mEntryMap;
		fw::Stopwatch mRemoveSW;
		long long mRemoveFreqMs = 10000LL;
	};
}
