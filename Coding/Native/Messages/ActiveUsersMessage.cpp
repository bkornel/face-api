#include "ActiveUsersMessage.h"

namespace face
{
	ActiveUsersMessage::ActiveUsersMessage(const UserVector& iActiveUsers, unsigned iFrameId, long long iTimestamp) :
		Message(iFrameId, iTimestamp)
	{
		for (const auto& user : iActiveUsers)
		{
			if (user && user->IsActive())
				mActiveUsers.push_back(user);
		}
	}

	void ActiveUsersMessage::RemoveInactiveUsers()
	{
		mActiveUsers.erase(
			std::remove_if(mActiveUsers.begin(), mActiveUsers.end(), [&](const User::Shared& obj)-> bool
		{
			return !obj->IsActive();
		}),
			mActiveUsers.end());
	}
}