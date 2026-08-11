#pragma once

#include "Framework/Module.h"

#include <memory>
#include <opencv2/core.hpp>
#include <map>

namespace face
{
  class ModuleConnector
  {
  public:
    using PredecessorMap = std::map<int, std::shared_ptr<fw::Module>>;

    static fw::ErrorCode Connect(std::shared_ptr<fw::Module> iModule, const PredecessorMap& iPredecessors);
  };
}