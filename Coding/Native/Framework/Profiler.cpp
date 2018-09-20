#include "Profiler.h"

#include <fstream>
#include <functional>
#include <opencv2/core/core.hpp>
#include <easyloggingpp/easyloggingpp.h>

namespace fw
{
	Profiler::Profiler(const std::string& iName) :
		mName(iName)
	{
		mStopwatch.Start();
	}

	Profiler::~Profiler()
	{
		mStopwatch.Stop();
		ProfilerDatabase::GetInstance().Push(mName, mStopwatch.GetElapsedTimeMilliSec());
	}

	std::recursive_mutex ProfilerDatabase::sMutex;

	ProfilerDatabase& ProfilerDatabase::GetInstance()
	{
		static ProfilerDatabase sInstance;
		return sInstance;
	}

	void ProfilerDatabase::Push(const std::string& iName, double iMilliseconds)
	{
		std::lock_guard<std::recursive_mutex> lock(sMutex);
		const std::size_t nameHash = std::hash<std::string>{}(iName);
		if (mNames.empty() || mNames.find(nameHash) == mNames.end())
			mNames[nameHash] = iName;

		mMeasurements[nameHash].push_back({ mCurrentFrameId, iMilliseconds });
	}

	void ProfilerDatabase::Save(const std::string& iPath) const
	{
		std::ofstream outFile(iPath);

		LOG(INFO) << "Profiler is saving: " << iPath;

		if (outFile.is_open())
		{
			std::lock_guard<std::recursive_mutex> lock(sMutex);

			for (const auto& m : mMeasurements)
			{
				const std::size_t hash = m.first;
				const auto& data = m.second;

				auto itName = mNames.find(hash);
				if (itName != mNames.end())
					outFile << itName->second << std::endl;

				outFile << "frame_id:" << "\t";
				for (const auto& d : data)
					outFile << d.first << "\t";

				outFile << std::endl << "runtime_ms:" << "\t";
				for (const auto& d : data)
					outFile << cvRound(d.second) << "\t";
			}

			outFile.flush();
			outFile.close();
		}
	}

	std::map<std::string, ProfilerDatabase::Measurement> ProfilerDatabase::GetLastMeasurement() const
	{
		std::lock_guard<std::recursive_mutex> lock(sMutex);
		std::map<std::string, Measurement> lastMeasurement;

		for (const auto& m : mMeasurements)
		{
			const std::size_t hash = m.first;

			auto itName = mNames.find(hash);
			if (itName != mNames.end())
			{
				const auto& data = m.second;
				if (!data.empty())
					lastMeasurement[itName->second] = data.back();
			}
		}

		return lastMeasurement;
	}
}