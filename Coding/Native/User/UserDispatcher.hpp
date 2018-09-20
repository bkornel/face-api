#pragma once

namespace face
{
	class User;

	class UserDispatcher
	{
	public:
		UserDispatcher() = default;

		virtual ~UserDispatcher() = default;

		virtual fw::ErrorCode Initialize(const cv::FileNode& iSettings) = 0;

		virtual bool Dispatch(User& ioUser) = 0;

	private:
		UserDispatcher(const UserDispatcher& iOther) = delete;

		UserDispatcher& operator=(const UserDispatcher& iOther) = delete;
	};
}