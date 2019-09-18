#pragma once

#include "Framework/Module.h"

#include <opencv2/core.hpp>
#include <map>

namespace face
{
  class ModuleConnector
  {
  public:
    using PredecessorMap = std::map<int, fw::Module::Shared>;

    static fw::ErrorCode Connect(fw::Module::Shared iModule, const PredecessorMap& iPredecessors);
  };
}