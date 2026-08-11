#pragma once


#include "Framework/ErrorCode.h"
#include <atomic>
#include <memory>
#include <future>

namespace fw
{
  class Thread
  {
  public:
    Thread() = default;

    Thread(const Thread& iOther) = delete;

    virtual ~Thread();

    Thread& operator=(const Thread& iOther) = delete;

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
    std::future<ErrorCode> mThread;
    std::atomic<bool> mStopThread{ false };
    std::atomic<bool> mFirstRun{ true };
  };
}
