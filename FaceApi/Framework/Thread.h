#pragma once

#include "Framework/ErrorCode.h"

#include <atomic>
#include <chrono>
#include <thread>

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

    bool GetThreadStopSignal() const;

    void StopSignalThread();

  protected:
    virtual ErrorCode Run();

  private:
    // std::jthread carries the stop token and joins in its own destructor, which is what the
    // stop flag plus the future and its wait() were doing by hand.
    std::jthread mThread;

    // jthread cannot say whether the thread function has returned, only whether a thread
    // object is attached, and Run() may finish on its own without a stop being requested
    std::atomic<bool> mRunning{ false };
  };
}
