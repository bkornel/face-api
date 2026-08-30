#include "Framework/Stopwatch.h"

namespace fw
{
  Stopwatch::Stopwatch(bool iStart /*= false*/)
  {
    Reset();

    if (iStart) Start();
  }

  void Stopwatch::Start()
  {
    mStartTime = Clock::now();
    mIsRunning = true;
  }

  void Stopwatch::Stop()
  {
    mStopTime = Clock::now();
    mIsRunning = false;
  }

  void Stopwatch::Reset()
  {
    mConstructionTime = mStartTime = mStopTime = Clock::now();
  }

  Milliseconds Stopwatch::GetElapsed(bool iStopped) const
  {
    const Clock::time_point end = iStopped ? mStopTime : Clock::now();
    const Milliseconds elapsed = end - mStartTime;

    // steady_clock cannot go backwards, but Stop() may predate the last Start()
    return elapsed.count() < 0.0 ? Milliseconds::zero() : elapsed;
  }

  double Stopwatch::GetFPS(bool iStopped) const
  {
    const double seconds = GetElapsedTimeSec(iStopped);
    return seconds > 0.0 ? 1.0 / seconds : 0.0;
  }

  double Stopwatch::GetElapsedTimeSec(bool iStopped) const
  {
    return std::chrono::duration_cast<Seconds>(GetElapsed(iStopped)).count();
  }

  double Stopwatch::GetElapsedTimeMilliSec(bool iStopped) const
  {
    return GetElapsed(iStopped).count();
  }

  double Stopwatch::GetElapsedTimeFromConstructionSec(bool iStopped) const
  {
    const Clock::time_point end = iStopped ? mStopTime : Clock::now();
    const Seconds elapsed = end - mConstructionTime;

    return elapsed.count() < 0.0 ? 0.0 : elapsed.count();
  }
}
