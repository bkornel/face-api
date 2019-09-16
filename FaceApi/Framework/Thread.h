#pragma once

#include "Framework/Util.h"

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
    volatile bool mStopThread = false;
    volatile bool mFirstRun = true;
  };
}
