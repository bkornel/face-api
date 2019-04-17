#pragma once

#include <memory>
#include <future>

#include "Framework/Util.h"

namespace fw
{
	class Thread
	{
	public:
		Thread() = default;

		virtual ~Thread();

		ErrorCode StartThread();

		ErrorCode StopThread();

		void ThreadSleep(long long iMilliseconds);

		bool IsRunning() const;

		inline bool GetThreadStopSignal() const
		{
			return mStopThread;
		}

		inline void StopSignalThread()
		{
			mStopThread = true;
		}

	protected:
		virtual ErrorCode Run();

	private:
		Thread(const Thread& iOther) = delete;

		Thread& operator=(const Thread& iOther) = delete;

		std::future<ErrorCode> mThread;
		volatile bool mStopThread = false;
		volatile bool mFirstRun = true;
	};
}
