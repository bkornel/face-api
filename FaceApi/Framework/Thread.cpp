#include "Framework/Thread.h"

namespace fw
{
  Thread::~Thread()
  {
    StopThread();
  }

  ErrorCode Thread::Run()
  {
    return ErrorCode::OK;
  }

  ErrorCode Thread::StartThread()
  {
    if (!mFirstRun) StopThread();

    mStopThread = mFirstRun = false;
    mThread = std::async(std::launch::async, &Thread::Run, this);

    return ErrorCode::OK;
  }

  fw::ErrorCode Thread::StopThread()
  {
    if (IsRunning())
    {
      StopSignalThread();
      mThread.wait();
      return ErrorCode::OK;
    }

    return ErrorCode::BadState;
  }

  void Thread::ThreadSleep(long long iMilliseconds)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(iMilliseconds));
  }

  bool Thread::IsRunning() const
  {
    return !mFirstRun && mThread.wait_for(std::chrono::seconds(0)) != std::future_status::ready;
  }
}
