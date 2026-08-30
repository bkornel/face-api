#include "Framework/ErrorCode.h"
#include "Framework/Thread.h"
#include "Framework/TimeExtensions.h"

#include <cstdint>
#include <easyloggingpp/easyloggingpp.h>

namespace fw
{
  Thread::~Thread()
  {
    // Derived classes must stop the thread while their own state is still alive. jthread
    // would join here by itself, but by then Run() would be working against a destroyed
    // derived object, so the warning still earns its place.
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
    if (mThread.joinable()) StopThread();

    mRunning = true;

    // Assigning over a jthread requests a stop on the previous one and joins it
    mThread = std::jthread([this](std::stop_token /*iStopToken*/) {
      Run();
      mRunning = false;
    });

    return ErrorCode::OK;
  }

  ErrorCode Thread::StopThread()
  {
    if (!mThread.joinable()) return ErrorCode::BadState;

    StopSignalThread();
    mThread.join();

    return ErrorCode::OK;
  }

  void Thread::ThreadSleep(int64_t iMilliseconds)
  {
    std::this_thread::sleep_for(Milliseconds(iMilliseconds));
  }

  bool Thread::IsRunning() const
  {
    return mRunning && !mThread.get_stop_token().stop_requested();
  }

  bool Thread::GetThreadStopSignal() const
  {
    return mThread.get_stop_token().stop_requested();
  }

  void Thread::StopSignalThread()
  {
    mThread.request_stop();
  }
}
