#pragma once

#include "Framework/Util.h"

#include <atomic>
#include <memory>
#include <future>

namespace fw
{
  /// @brief Base class running Run() on a worker thread.
  /// IMPORTANT: Run() is virtual, so every derived class must stop the thread in
  /// its own destructor (directly or through DeInitialize). By the time ~Thread()
  /// runs, the derived part of the object is already gone and a still-running
  /// Run() would touch destroyed state. The StopThread() in ~Thread() is only a
  /// last resort and logs a warning if it actually had to do something.
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
