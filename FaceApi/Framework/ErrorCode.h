#pragma once

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
}
