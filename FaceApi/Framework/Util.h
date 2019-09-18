#pragma once

#include <limits>
#include <memory>
#include <string>

#define FW_PLUGIN_NAME ("Face")

#define FW_DEG_TO_RAD(degree) ((degree) * (M_PI / 180.0))
#define FW_RAD_TO_DEG(radian) ((radian) * (180.0 / M_PI))

#define	FW_DEFINE_SMART_POINTERS(C)             \
	using Unique = std::unique_ptr<C> ;           \
	using ConstUnique = std::unique_ptr<const C>; \
	using Shared = std::shared_ptr<C>;            \
	using ConstShared = std::shared_ptr<const C>

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

  using ResultCode = ErrorCode;

  long long get_current_time();

  std::string& get_log_stamp();

  template<typename T>
  T scale_interval(T iValueIn, T iBaseMin, T iBaseMax, T iLimitMin, T iLimitMax)
  {
    return static_cast<T>((((float)iLimitMax - (float)iLimitMin) * ((float)iValueIn - (float)iBaseMin) /
      ((float)iBaseMax - (float)iBaseMin)) + (float)iLimitMin);
  }

  template<typename T>
  inline bool equals(T iA, T iB)
  {
    return abs(iA - iB) <= std::numeric_limits<T>::epsilon();
  }

  template<typename ContainerT, typename PredicateT>
  void remove_if(ContainerT& iItems, const PredicateT& iPredicate)
  {
    for (auto it = iItems.begin(); it != iItems.end(); )
    {
      if (iPredicate(*it)) it = iItems.erase(it);
      else ++it;
    }
  };
}
