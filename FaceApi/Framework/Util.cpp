#include "Framework/Util.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <ctime>
#include <chrono>

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

	long long get_current_time()
	{
		using namespace std::chrono;
		milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		return ms.count();
	}

	std::string& get_log_stamp()
	{
		static std::string sLogStamp = generate_log_stamp();
		return sLogStamp;
	}
}
