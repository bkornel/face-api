#pragma once

#include "Framework/Stopwatch.h"

#include <deque>
#include <map>
#include <vector>
#include <string>
#include <mutex>

#if !defined(ENABLE_FACE_PROFILER) && !defined(FACE_PROFILER_DISABLED)
  #define ENABLE_FACE_PROFILER
#endif

// Profiler is enabled
#ifdef ENABLE_FACE_PROFILER
  #define FACE_PROFILER(name) fw::Profiler _FaceProfiler_##name##__LINE__(#name)
  #define FACE_PROFILER_FRAME_ID(frameId) fw::ProfilerDatabase::GetInstance().setCurrentFrameId(frameId)
  #define FACE_PROFILER_SAVE(name) fw::ProfilerDatabase::GetInstance().Save(name)
  #define FACE_PROFILER_SUMMARY() fw::ProfilerDatabase::GetInstance().LogStatistics()
// Profiler is disabled
#else
  #define FACE_PROFILER(name)
  #define FACE_PROFILER_FRAME_ID(frameId)
  #define FACE_PROFILER_SAVE(name)
  #define FACE_PROFILER_SUMMARY()
#endif

namespace fw
{
  class Profiler
  {
  public:
    explicit Profiler(const std::string& iName);

    ~Profiler();

  private:
    std::string mName;
    Stopwatch mStopwatch;
  };

  class ProfilerDatabase
  {
  public:
    using Measurement = std::pair<unsigned, double>;

    struct Statistics
    {
      std::size_t count = 0U;
      double min = 0.0;
      double max = 0.0;
      double mean = 0.0;
      double median = 0.0;
      double p95 = 0.0;
      double total = 0.0;
    };

    static ProfilerDatabase& GetInstance();

    void Push(const std::string& iName, double iMilliseconds);

    void Save(const std::string& iName) const;

    std::map<std::string, Measurement> GetLastMeasurement() const;

    std::map<std::string, Statistics> GetStatistics() const;

    std::string FormatStatistics() const;

    void LogStatistics() const;

    void setCurrentFrameId(unsigned iCurrentFrameId);

  private:
    static std::recursive_mutex sMutex;

    static const std::size_t sMaxSamplesPerName;

    ProfilerDatabase() = default;

    ProfilerDatabase(const ProfilerDatabase& iOther) = delete;

    ProfilerDatabase& operator=(const ProfilerDatabase& iOther) = delete;

    unsigned mCurrentFrameId = 0U;
    std::map<std::size_t, std::string> mNames;
    std::map<std::size_t, std::deque<Measurement>> mMeasurements;
  };
}
