#include "Framework/Stopwatch.h"

#include <opencv2/core/core.hpp>

#define TICK_COUNT (cv::getTickCount())
#define TICK_FREQUENCY (cv::getTickFrequency())

namespace fw
{
	Stopwatch::Stopwatch(bool iStart /*= false*/)
	{
		Reset();

		if (iStart) Start();
	}

	void Stopwatch::Start()
	{
		mStartTime = TICK_COUNT;
		mIsRunning = true;
	}

	void Stopwatch::Stop()
	{
		mStopTime = TICK_COUNT;
		mIsRunning = false;
	}

	void Stopwatch::Reset()
	{
		mConstructionTime = mStartTime = mStopTime = TICK_COUNT;
	}

	double Stopwatch::GetFPS(bool stopped) const
	{
		const long long diff = std::abs(stopped ? mStopTime - mStartTime : TICK_COUNT - mStartTime);
		return diff > 0 ? 1.0 / (diff / TICK_FREQUENCY) : 0.0;
	}

	double Stopwatch::GetElapsedTimeSec(bool stopped) const
	{
		const long long diff = std::abs(stopped ? mStopTime - mStartTime : TICK_COUNT - mStartTime);
		return diff > 0 ? (diff / TICK_FREQUENCY) : 0.0;
	}

	double Stopwatch::GetElapsedTimeMilliSec(bool stopped) const
	{
		return GetElapsedTimeSec(stopped) * 1000.0;
	}

	double Stopwatch::GetElapsedTimeFromConstructionSec(bool stopped) const
	{
		const long long diff = std::abs(stopped ? mStopTime - mConstructionTime : TICK_COUNT - mConstructionTime);
		return diff > 0 ? (diff / TICK_FREQUENCY) : 0.0;
	}
}