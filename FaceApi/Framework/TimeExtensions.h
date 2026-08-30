#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace fw
{
  /// @brief A point in wall clock time, which is what a frame is stamped with: the stamp has
  /// to mean something outside this process, since it names log files and crosses over to the
  /// host application. Measuring an interval *inside* the process is fw::Stopwatch's job, and
  /// that one runs on steady_clock so a clock correction cannot bend the measurement.
  using WallClock = std::chrono::system_clock;
  using Timestamp = WallClock::time_point;

  /// @brief Fractional units of time. The unit lives in the type, so nothing has to be named
  /// somethingMs to say what it holds.
  using Seconds = std::chrono::duration<double>;
  using Milliseconds = std::chrono::duration<double, std::milli>;
  using Microseconds = std::chrono::duration<double, std::micro>;

  inline Timestamp now()
  {
    return WallClock::now();
  }

  /// @brief Time between two stamps, never negative
  Milliseconds elapsed(Timestamp iFrom, Timestamp iTo);

  Milliseconds elapsed_since(Timestamp iFrom);

  /// @brief For the boundaries that can only carry a number: the JNI buffer, FaceResult, logs
  int64_t to_epoch_ms(Timestamp iTimestamp);

  Timestamp from_epoch_ms(int64_t iMilliseconds);

  std::string& get_log_stamp();
}
