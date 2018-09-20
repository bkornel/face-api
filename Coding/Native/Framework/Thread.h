#pragma once

#include <memory>
#include <future>

#include "Util.h"

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
		virtual ErrorCode ThreadProcedure();

	private:
		Thread(const Thread& iOther) = delete;

		Thread& operator=(const Thread& iOther) = delete;

		std::future<ErrorCode> mThread;
		volatile bool mStopThread = false;
		volatile bool mFirstRun = true;
	};
}
