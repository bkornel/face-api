#include "Framework/TimeExtensions.h"

#include <format>

namespace fw
{
  namespace
  {
    std::string generate_log_stamp()
    {
      // std::format over chrono, which replaces the strftime call that needed one branch for
      // localtime_s and another for localtime.
      //
      // REMARK: current_zone() reads the time zone database. MSVC ships it, and libc++ only
      // gained it recently, so an older NDK may not build this. Formatting the time point
      // directly instead of the zoned_time drops the requirement, at the cost of the stamp
      // being UTC rather than local.
      const auto seconds = std::chrono::floor<std::chrono::seconds>(now());
      const std::chrono::zoned_time local{ std::chrono::current_zone(), seconds };

      return std::format("{:%Y_%m_%d-%H_%M_%S}", local);
    }
  }

  Milliseconds elapsed(Timestamp iFrom, Timestamp iTo)
  {
    const Milliseconds difference = iTo - iFrom;

    // A frame stamped slightly in the future is possible when the host supplies the stamp,
    // and every caller here wants a length rather than a direction
    return difference.count() < 0.0 ? Milliseconds::zero() : difference;
  }

  Milliseconds elapsed_since(Timestamp iFrom)
  {
    return elapsed(iFrom, now());
  }

  long long to_epoch_ms(Timestamp iTimestamp)
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(iTimestamp.time_since_epoch()).count();
  }

  Timestamp from_epoch_ms(long long iMilliseconds)
  {
    return Timestamp(std::chrono::milliseconds(iMilliseconds));
  }

  // One stamp per process run: it names the log and the profiler file of this run
  std::string& get_log_stamp()
  {
    static std::string sLogStamp = generate_log_stamp();
    return sLogStamp;
  }
}
