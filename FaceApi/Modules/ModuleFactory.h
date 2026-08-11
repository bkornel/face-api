#pragma once

#include "Framework/Module.h"
#include <memory>
#include <opencv2/core.hpp>

namespace face
{
  class ModuleFactory
  {
  public:
    static std::shared_ptr<fw::Module> Create(const cv::FileNode& iModuleNode, fw::MessageBus& ioBus);
  };
}
