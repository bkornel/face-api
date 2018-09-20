#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <ctime>
#include <chrono>

#include "Util.h"

namespace fw
{
	namespace
	{
		std::string generate_log_stamp()
		{
			char buffer[100];

#if defined(WIN32)
			time_t rawtime = std::time(nullptr);
			struct tm timeinfo;

			localtime_s(&timeinfo, &rawtime);
			std::strftime(buffer, 100, "%Y_%m_%d-%H_%M_%S", &timeinfo);
#elif defined(__ANDROID__)
			time_t rawtime = std::time(nullptr);
			struct tm* timeinfo = localtime(&rawtime);
			std::strftime(buffer, 100, "%Y_%m_%d-%H_%M_%S", timeinfo);
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