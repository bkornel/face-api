#pragma once

namespace fw
{
  class Stopwatch
  {
  public:
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

    inline bool IsRunning() const
    {
      return mIsRunning;
    }

  private:
    long long mConstructionTime = 0;
    long long mStopTime = 0;
    long long mStartTime = 0;
    bool mIsRunning = false;
  };
}
