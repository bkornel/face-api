#pragma once

#include <limits>
#include <memory>
#include <string>

#define FW_PLUGIN_NAME ("Face")

#define FW_DEG_TO_RAD(degree) ((degree) * (M_PI / 180.0))
#define FW_RAD_TO_DEG(radian) ((radian) * (180.0 / M_PI))

#define	FW_DEFINE_SMART_POINTERS(C) \
	using Unique = std::unique_ptr<C> ; \
	using ConstUnique = std::unique_ptr<const C>; \
	using Shared = std::shared_ptr<C>; \
	using ConstShared = std::shared_ptr<const C>;

namespace fw
{
	enum class ErrorCode
	{
		OK = 0,
		SystemFailure = 1,
		FatalFailure = 2,
		NotFound = 3,
		OutOfResources = 4,
		BadData = 5,
		BadState = 6,
		NotSupported = 7,
		OutOfMemory = 8,
		BadParam = 9
	};

	typedef ErrorCode ResultCode;

	long long get_current_time();

	std::string& get_log_stamp();

	template<typename _Tp>
	_Tp scale_interval(_Tp valueIn, _Tp baseMin, _Tp baseMax, _Tp limitMin, _Tp limitMax)
	{
		return static_cast<_Tp>((((float)limitMax - (float)limitMin) * ((float)valueIn - (float)baseMin) /
			((float)baseMax - (float)baseMin)) + (float)limitMin);
	}

	template<typename _Tp>
	inline bool equals(_Tp a, _Tp b)
	{
		return abs(a - b) <= std::numeric_limits<_Tp>::epsilon();
	}

	template<typename ContainerT, typename PredicateT>
	void remove_if(ContainerT& items, const PredicateT& predicate)
	{
		for (auto it = items.begin(); it != items.end(); )
		{
			if (predicate(*it)) it = items.erase(it);
			else ++it;
		}
	};
}
