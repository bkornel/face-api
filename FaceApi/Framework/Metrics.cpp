#include "Framework/Metrics.h"

#include <algorithm>
#include <cmath>

namespace fw
{
  double percentile_of(std::vector<double>& ioSamples, double iRatio)
  {
    if (ioSamples.empty()) return 0.0;

    const double clamped = std::clamp(iRatio, 0.0, 1.0);
    const std::size_t index = static_cast<std::size_t>(clamped * (ioSamples.size() - 1U) + 0.5);

    std::nth_element(ioSamples.begin(), ioSamples.begin() + static_cast<std::ptrdiff_t>(index), ioSamples.end());

    return ioSamples[index];
  }

  RateCounter::RateCounter(int64_t iWindowMs) :
    mWindowMs(iWindowMs > 0LL ? iWindowMs : 1000LL)
  {
  }

  void RateCounter::Tick(int64_t iNowMs)
  {
    mEvents.emplace_back(iNowMs);
    Trim(iNowMs);
  }

  void RateCounter::Update(int64_t iNowMs)
  {
    Trim(iNowMs);
  }

  void RateCounter::Trim(int64_t iNowMs)
  {
    const int64_t oldest = iNowMs - mWindowMs;

    while (!mEvents.empty() && mEvents.front() < oldest) mEvents.pop_front();

    // Until the window has filled up, the rate is measured over what there is instead of
    // over the nominal window, which would otherwise read half the true rate at startup
    const int64_t span = mEvents.empty() ? 0LL : (iNowMs - mEvents.front());
    const double effective = static_cast<double>((std::max)(span, mWindowMs / 4LL));

    mRate = effective > 0.0 ? mEvents.size() * 1000.0 / effective : 0.0;
  }

  void RateCounter::Reset()
  {
    mEvents.clear();
    mRate = 0.0;
  }

  SampleStatistics::SampleStatistics(std::size_t iCapacity) :
    mCapacity(iCapacity > 0U ? iCapacity : 1U)
  {
  }

  void SampleStatistics::Push(double iValue)
  {
    if (!std::isfinite(iValue)) return;

    if (mSamples.size() >= mCapacity)
    {
      mSum -= mSamples.front();
      mSamples.pop_front();
    }

    mSamples.emplace_back(iValue);
    mSum += iValue;
    mLast = iValue;

    // Over the history, not over the session: a single stall at startup would otherwise sit
    // in the maximum for as long as the application runs
    const auto minmax = std::minmax_element(mSamples.begin(), mSamples.end());
    mMinimum = *minmax.first;
    mMaximum = *minmax.second;
  }

  void SampleStatistics::Reset()
  {
    mSamples.clear();
    mSorted.clear();

    mLast = 0.0;
    mMinimum = 0.0;
    mMaximum = 0.0;
    mSum = 0.0;
  }

  double SampleStatistics::GetMean() const
  {
    return mSamples.empty() ? 0.0 : mSum / static_cast<double>(mSamples.size());
  }

  double SampleStatistics::GetPercentile(double iPercentile) const
  {
    mSorted.assign(mSamples.begin(), mSamples.end());
    return percentile_of(mSorted, iPercentile);
  }
}
