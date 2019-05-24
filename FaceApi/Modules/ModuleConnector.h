#pragma once

#include "Framework/Module.h"

#include <opencv2/core.hpp>
#include <map>

namespace face
{
  class ModuleConnector
  {
    using PredecessorMap = std::map<int, fw::Module::Shared>;

  public:
    static fw::ErrorCode Connect(fw::Module::Shared iModule, const PredecessorMap& iPredecessors);
  };
}