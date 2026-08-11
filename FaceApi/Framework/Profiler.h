#pragma once

#include "Framework/Stopwatch.h"

#include <deque>
#include <map>
#include <vector>
#include <string>
#include <mutex>

// Enabled by default. Define FACE_PROFILER_DISABLED from the build system to
// compile the profiler out completely.
#if !defined(ENABLE_FACE_PROFILER) && !defined(FACE_PROFILER_DISABLED)
  #define ENABLE_FACE_PROFILER
#endif

// Profiler is enabled
#ifdef ENABLE_FACE_PROFILER
  #define FACE_PROFILER(name) fw::Profiler _FaceProfiler_##name##__LINE__(#name)
  #define FACE_PROFILER_FRAME_ID(frameId) fw::ProfilerDatabase::GetInstance().setCurrentFrameId(frameId)
  #define FACE_PROFILER_SAVE(name) fw::ProfilerDatabase::GetInstance().Save(name)
// Profiler is disabled
#else
  #define FACE_PROFILER(name)
  #define FACE_PROFILER_FRAME_ID(frameId)
  #define FACE_PROFILER_SAVE(name)
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

    static ProfilerDatabase& GetInstance();

    void Push(const std::string& iName, double iMilliseconds);

    void Save(const std::string& iName) const;

    std::map<std::string, Measurement> GetLastMeasurement() const;

    inline void setCurrentFrameId(unsigned iCurrentFrameId)
    {
      mCurrentFrameId = iCurrentFrameId;
    }

  private:
    static std::recursive_mutex sMutex;

    /// @brief Upper bound of the kept samples per measured scope. The database is
    /// only written out on shutdown, so without a bound a long running session
    /// (a mobile app in particular) would grow it for as long as it lives.
    static const std::size_t sMaxSamplesPerName;

    ProfilerDatabase() = default;

    ProfilerDatabase(const ProfilerDatabase& iOther) = delete;

    ProfilerDatabase& operator=(const ProfilerDatabase& iOther) = delete;

    unsigned mCurrentFrameId = 0U;
    std::map<std::size_t, std::string> mNames;
    std::map<std::size_t, std::deque<Measurement>> mMeasurements;
  };
}
