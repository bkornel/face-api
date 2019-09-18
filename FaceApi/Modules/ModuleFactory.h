#pragma once

#include "Framework/Module.h"
#include <opencv2/core.hpp>

namespace face
{
  class ModuleFactory
  {
    public:
      static fw::Module::Shared Create(const cv::FileNode& iModuleNode);
  };
}
