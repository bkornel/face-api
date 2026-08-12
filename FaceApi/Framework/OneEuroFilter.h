#pragma once

#include <cmath>
#include <numbers>

namespace fw
{
  /// @brief The One Euro filter of Casiez et al.: an adaptive low-pass whose cutoff rises
  /// with the speed of the signal, so slow signals are smoothed hard and fast motion is
  /// followed with little lag. The usual cure for per-frame regression jitter.
  class OneEuroFilter
  {
  public:
    OneEuroFilter(double iMinCutoff, double iBeta, double iDerivativeCutoff = 1.0) :
      mMinCutoff(iMinCutoff),
      mBeta(iBeta),
      mDerivativeCutoff(iDerivativeCutoff)
    {
    }

    double Filter(double iValue, double iDtSec)
    {
      if (iDtSec <= 0.0) return mHasLast ? mLastValue : iValue;

      if (!mHasLast)
      {
        mHasLast = true;
        mLastValue = iValue;
        mLastDerivative = 0.0;
        return iValue;
      }

      const double derivative = (iValue - mLastValue) / iDtSec;
      mLastDerivative = Lowpass(derivative, mLastDerivative, Alpha(mDerivativeCutoff, iDtSec));

      const double cutoff = mMinCutoff + mBeta * std::abs(mLastDerivative);
      mLastValue = Lowpass(iValue, mLastValue, Alpha(cutoff, iDtSec));

      return mLastValue;
    }

    void Reset()
    {
      mHasLast = false;
    }

  private:
    static double Alpha(double iCutoff, double iDtSec)
    {
      const double tau = 1.0 / (2.0 * std::numbers::pi * iCutoff);
      return 1.0 / (1.0 + tau / iDtSec);
    }

    static double Lowpass(double iValue, double iLast, double iAlpha)
    {
      return iAlpha * iValue + (1.0 - iAlpha) * iLast;
    }

    double mMinCutoff = 1.0;
    double mBeta = 0.0;
    double mDerivativeCutoff = 1.0;

    bool mHasLast = false;
    double mLastValue = 0.0;
    double mLastDerivative = 0.0;
  };
}
