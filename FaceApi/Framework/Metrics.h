/// @file Metrics.h
/// @brief Measuring a rate and a distribution over a moving window.
///
/// Next to Stopwatch and Profiler, which measure one interval and one stage: these two
/// measure a stream of them, which is what a host needs to say how the pipeline is doing
/// rather than how one frame went.

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace fw
{
  /// @brief The iRatio quantile of ioSamples, in [0, 1]. Reorders ioSamples in place -
  /// nth_element puts only the element that is asked for into its sorted position, which is
  /// all a quantile needs.
  double percentile_of(std::vector<double>& ioSamples, double iRatio);

  /// @brief Events per second, measured over a sliding window rather than from the gap
  /// between the last two events.
  ///
  /// A frame rate read off one interval jitters by whole frames and is unreadable; a window
  /// gives the number a person would count. The window is in time, not in samples, so the
  /// rate falls towards zero when the events stop instead of freezing at the last value.
  class RateCounter
  {
  public:
    explicit RateCounter(int64_t iWindowMs = 1000LL);

    void Tick(int64_t iNowMs);

    /// @brief Ages the window out without recording an event, so that a stall shows up
    void Update(int64_t iNowMs);

    inline double GetRate() const
    {
      return mRate;
    }

    void Reset();

  private:
    void Trim(int64_t iNowMs);

    int64_t mWindowMs;
    std::deque<int64_t> mEvents;
    double mRate = 0.0;
  };

  /// @brief The distribution of a per-frame measurement - a latency, a stage time - kept
  /// over a bounded history so that the numbers describe now rather than the whole session.
  class SampleStatistics
  {
  public:
    explicit SampleStatistics(std::size_t iCapacity = 240U);

    void Push(double iValue);

    void Reset();

    inline bool IsEmpty() const
    {
      return mSamples.empty();
    }

    inline double GetLast() const
    {
      return mLast;
    }

    inline double GetMinimum() const
    {
      return mMinimum;
    }

    inline double GetMaximum() const
    {
      return mMaximum;
    }

    double GetMean() const;

    /// @param iPercentile In [0, 1]; 0.95 is the value 95 per cent of the samples are under
    double GetPercentile(double iPercentile) const;

    /// @brief The history itself, oldest first, for a host that wants to plot it
    inline const std::deque<double>& GetSamples() const
    {
      return mSamples;
    }

  private:
    std::size_t mCapacity;
    std::deque<double> mSamples;

    /// @brief Reused by GetPercentile so that reading a statistic does not allocate
    mutable std::vector<double> mSorted;

    double mLast = 0.0;
    double mMinimum = 0.0;
    double mMaximum = 0.0;
    double mSum = 0.0;
  };
}
