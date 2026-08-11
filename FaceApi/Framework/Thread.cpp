#include "Framework/ErrorCode.h"
#include "Framework/Thread.h"

#include <easyloggingpp/easyloggingpp.h>

namespace fw
{
  Thread::~Thread()
  {
    // Derived classes must stop the thread while their own state is still alive.
    if (IsRunning())
    {
      LOG(WARNING) << "Thread was still running in ~Thread(); the derived class should have stopped it.";
      StopThread();
    }
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
