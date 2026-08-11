#include "Framework/ErrorCode.h"
#include "Modules/ModuleConnector.h"

#include "Framework/Graph/IPortConnector.h"

#include <easyloggingpp/easyloggingpp.h>

#include <sstream>

namespace face
{
  fw::ErrorCode ModuleConnector::Connect(std::shared_ptr<fw::Module> iModule, const PredecessorMap& iPredecessors)
  {
    CV_DbgAssert(iModule);

    // Every module that takes part in the graph declares its ports through fw::Port, which is
    // where this interface comes from, so nothing below has to know which module it is holding
    auto ports = std::dynamic_pointer_cast<fw::IPortConnector>(iModule);
    if (!ports)
    {
      LOG(ERROR) << "Module declares no port: " << iModule->GetName();
      return fw::ErrorCode::BadState;
    }

    std::stringstream ss;
    ss << "Predecessors of [" << iModule->GetName() << "]:\t";

    if (iPredecessors.empty())
    {
      ss << "---";
    }

    for (const auto& [portNumber, predecessor] : iPredecessors)
    {
      if (!predecessor)
      {
        LOG(ERROR) << "Predecessor is not defined for module: " << iModule->GetName();
        return fw::ErrorCode::BadData;
      }

      auto predecessorPorts = std::dynamic_pointer_cast<fw::IPortConnector>(predecessor);
      if (!predecessorPorts)
      {
        LOG(ERROR) << "Predecessor declares no output port: " << predecessor->GetName();
        return fw::ErrorCode::BadData;
      }

      // Port numbers start from 1 in the settings file
      const std::size_t portIndex = static_cast<std::size_t>(portNumber - 1);

      if (ports->SetInput(portIndex, predecessorPorts->GetOutput()) != fw::ErrorCode::OK)
      {
        LOG(ERROR) << "Connection cannot be created: " << predecessor->GetName() << " -> "
                   << iModule->GetName() << " on port " << portNumber;
        return fw::ErrorCode::BadData;
      }

      ss << "[" << predecessor->GetName() << ":" << portNumber << "]\t";
    }

    LOG(INFO) << ss.str();

    return ports->Connect();
  }
}
