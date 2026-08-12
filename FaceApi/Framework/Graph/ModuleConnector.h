#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"

#include <map>
#include <memory>

namespace fw
{
  class ModuleConnector
  {
  public:
    using PredecessorMap = std::map<int, std::shared_ptr<Module>>;

    static ErrorCode Connect(const std::shared_ptr<Module>& iModule, const PredecessorMap& iPredecessors);
  };
}
