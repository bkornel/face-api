#pragma once

#include "Framework/TimeExtensions.h"

#include <chrono>

namespace fw
{
  class Stopwatch
  {
  public:
    // Measuring elapsed time is what steady_clock is for: it cannot be moved by the system
    // clock being corrected, and it never runs backwards
    using Clock = std::chrono::steady_clock;

    explicit Stopwatch(bool iStart = false);

    ~Stopwatch() = default;

    void Start();

    void Stop();

    //! Reset timer.
    void Reset();

    //! Calculate frame per second from time delay.
    double GetFPS(bool iStopped = true) const;

    //! Returns the elapsed time between the declaration of object (or since the last call of Reset method) and current time in second.
    double GetElapsedTimeSec(bool iStopped = true) const;

    //! Returns the elapsed time between the declaration of object (or since the last call of Reset method) and current time in milli second.
    double GetElapsedTimeMilliSec(bool iStopped = true) const;

    //! Returns the elapsed time between the declaration of object and current time in millisecond.
    double GetElapsedTimeFromConstructionSec(bool iStopped = true) const;

    //! The measured interval, for callers that would rather keep the unit in the type
    Milliseconds GetElapsed(bool iStopped = true) const;

    inline bool IsRunning() const
    {
      return mIsRunning;
    }

  private:
    Clock::time_point mConstructionTime = Clock::now();
    Clock::time_point mStopTime = Clock::now();
    Clock::time_point mStartTime = Clock::now();
    bool mIsRunning = false;
  };
}
