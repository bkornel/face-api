#include "Framework/TimeExtensions.h"

#include <ctime>

namespace fw
{
  namespace
  {
    std::string generate_log_stamp()
    {
      char buffer[100] = { 0 };
      time_t rawtime = std::time(nullptr);

#if defined(__ANDROID__)
      struct tm* timeinfo = localtime(&rawtime);
      std::strftime(buffer, 100, "%Y_%m_%d-%H_%M_%S", timeinfo);
#else
      struct tm timeinfo = { 0 };
      localtime_s(&timeinfo, &rawtime);
      std::strftime(buffer, 100, "%Y_%m_%d-%H_%M_%S", &timeinfo);
#endif

      return std::string(buffer);
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
