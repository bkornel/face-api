#include "Framework/Profiler.h"

#include <cstdint>
#include <easyloggingpp/easyloggingpp.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <vector>

namespace
{
  // iSorted must be sorted ascending
  double percentile(const std::vector<double>& iSorted, double iRatio)
  {
    if (iSorted.empty()) return 0.0;

    const std::size_t last = iSorted.size() - 1U;
    const std::size_t index = static_cast<std::size_t>((iRatio * last) + 0.5);

    return iSorted[(std::min)(index, last)];
  }
}

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

  const std::size_t ProfilerDatabase::sMaxSamplesPerName = 20000U;

  ProfilerDatabase& ProfilerDatabase::GetInstance()
  {
    static ProfilerDatabase sInstance;
    return sInstance;
  }

  void ProfilerDatabase::setCurrentFrameId(uint32_t iCurrentFrameId)
  {
    // Set from the app thread, read by Push() from the graph thread
    std::lock_guard<std::recursive_mutex> lock(sMutex);
    mCurrentFrameId = iCurrentFrameId;
  }

  void ProfilerDatabase::Push(const std::string& iName, double iMilliseconds)
  {
    std::lock_guard<std::recursive_mutex> lock(sMutex);
    const std::size_t nameHash = std::hash<std::string>{}(iName);
    if (mNames.empty() || mNames.find(nameHash) == mNames.end())
      mNames[nameHash] = iName;

    auto& samples = mMeasurements[nameHash];
    samples.emplace_back(mCurrentFrameId, iMilliseconds);

    // Keep the most recent window only, the oldest samples fall out.
    while (samples.size() > sMaxSamplesPerName)
      samples.pop_front();
  }

  void ProfilerDatabase::Save(const std::string& iPath) const
  {
    std::ofstream outFile(iPath);

    LOG(INFO) << "Profiler is saving: " << iPath;

    if (outFile.is_open())
    {
      // Aggregates first, the per-frame samples below are for plotting
      outFile << FormatStatistics() << std::endl;

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

        outFile << std::endl
                << "runtime_ms:" << "\t";

        // Three decimals: most stages run well under a millisecond, and rounding them to
        // whole milliseconds made the measurements useless
        outFile << std::fixed << std::setprecision(3);
        for (const auto& d : data)
          outFile << d.second << "\t";
      }

      outFile.flush();
      outFile.close();
    }
  }

  std::map<std::string, ProfilerDatabase::Statistics> ProfilerDatabase::GetStatistics() const
  {
    std::lock_guard<std::recursive_mutex> lock(sMutex);
    std::map<std::string, Statistics> result;

    for (const auto& m : mMeasurements)
    {
      auto itName = mNames.find(m.first);
      if (itName == mNames.end() || m.second.empty()) continue;

      std::vector<double> samples;
      samples.reserve(m.second.size());
      for (const auto& d : m.second)
        samples.emplace_back(d.second);

      std::sort(samples.begin(), samples.end());

      Statistics stats;
      stats.count = samples.size();
      stats.min = samples.front();
      stats.max = samples.back();
      stats.median = percentile(samples, 0.50);
      stats.p95 = percentile(samples, 0.95);
      stats.total = std::accumulate(samples.begin(), samples.end(), 0.0);
      stats.mean = stats.total / static_cast<double>(stats.count);

      result[itName->second] = stats;
    }

    return result;
  }

  std::string ProfilerDatabase::FormatStatistics() const
  {
    const std::map<std::string, Statistics> stats = GetStatistics();

    if (stats.empty()) return "Profiler: no measurements were collected.";

    // Most expensive first, that is the order one wants to read it in
    std::vector<std::pair<std::string, Statistics>> rows(stats.begin(), stats.end());
    std::sort(rows.begin(), rows.end(), [](const std::pair<std::string, Statistics>& iFirst,
                                           const std::pair<std::string, Statistics>& iSecond) {
      return iFirst.second.total > iSecond.second.total;
    });

    std::size_t nameWidth = 5U;
    for (const auto& r : rows)
      nameWidth = (std::max)(nameWidth, r.first.size());

    std::ostringstream os;
    os << "Profiler statistics, all times in milliseconds:" << std::endl;

    os << std::left << std::setw(static_cast<int>(nameWidth)) << "stage" << std::right
       << std::setw(9) << "calls"
       << std::setw(10) << "min"
       << std::setw(10) << "mean"
       << std::setw(10) << "median"
       << std::setw(10) << "p95"
       << std::setw(10) << "max"
       << std::setw(12) << "total" << std::endl;

    os << std::string(nameWidth + 71U, '-') << std::endl;

    os << std::fixed << std::setprecision(3);
    for (const auto& r : rows)
    {
      const Statistics& s = r.second;

      os << std::left << std::setw(static_cast<int>(nameWidth)) << r.first << std::right
         << std::setw(9) << s.count
         << std::setw(10) << s.min
         << std::setw(10) << s.mean
         << std::setw(10) << s.median
         << std::setw(10) << s.p95
         << std::setw(10) << s.max
         << std::setw(12) << s.total << std::endl;
    }

    return os.str();
  }

  void ProfilerDatabase::LogStatistics() const
  {
    LOG(INFO) << std::endl << FormatStatistics();
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
