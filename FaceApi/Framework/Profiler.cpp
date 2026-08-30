#include "Framework/Profiler.h"

#include "Framework/Metrics.h"

#include <cstdint>
#include <easyloggingpp/easyloggingpp.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <vector>

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

  const std::size_t ProfilerDatabase::sMaxSamplesPerName = 20000U;

  ProfilerDatabase& ProfilerDatabase::GetInstance()
  {
    static ProfilerDatabase sInstance;
    return sInstance;
  }

  void ProfilerDatabase::setCurrentFrameId(uint32_t iCurrentFrameId)
  {
    // Set from the app thread, read by Push() from the graph thread
    std::lock_guard<std::mutex> lock(mMutex);
    mCurrentFrameId = iCurrentFrameId;
  }

  void ProfilerDatabase::Push(const std::string& iName, double iMilliseconds)
  {
    std::lock_guard<std::mutex> lock(mMutex);

    auto& samples = mMeasurements[iName];
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

      std::lock_guard<std::mutex> lock(mMutex);

      for (const auto& [name, data] : mMeasurements)
      {
        outFile << name << std::endl;

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
    std::lock_guard<std::mutex> lock(mMutex);
    std::map<std::string, Statistics> result;

    for (const auto& [name, data] : mMeasurements)
    {
      if (data.empty()) continue;

      std::vector<double> samples;
      samples.reserve(data.size());
      for (const auto& d : data)
        samples.emplace_back(d.second);

      const auto minmax = std::minmax_element(samples.begin(), samples.end());

      Statistics stats;
      stats.count = samples.size();
      stats.min = *minmax.first;
      stats.max = *minmax.second;
      stats.total = std::accumulate(samples.begin(), samples.end(), 0.0);
      stats.mean = stats.total / static_cast<double>(stats.count);

      // percentile_of reorders the samples, so it comes after everything read in order
      stats.median = percentile_of(samples, 0.50);
      stats.p95 = percentile_of(samples, 0.95);

      result[name] = stats;
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
    std::lock_guard<std::mutex> lock(mMutex);
    std::map<std::string, Measurement> lastMeasurement;

    for (const auto& [name, data] : mMeasurements)
    {
      if (!data.empty())
        lastMeasurement[name] = data.back();
    }

    return lastMeasurement;
  }
}
