#include "Framework/Graph/ModuleConnector.h"

#include "Framework/Graph/IPortConnector.h"

#include <easyloggingpp/easyloggingpp.h>

#include <cassert>
#include <sstream>

namespace fw
{
  ErrorCode ModuleConnector::Connect(const std::shared_ptr<Module>& iModule, const PredecessorMap& iPredecessors)
  {
    assert(iModule);

    // Every module that takes part in the graph declares its ports through fw::Port, which is
    // where this interface comes from, so nothing below has to know which module it is holding
    auto ports = std::dynamic_pointer_cast<IPortConnector>(iModule);
    if (!ports)
    {
      LOG(ERROR) << "Module declares no port: " << iModule->GetName();
      return ErrorCode::BadState;
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
        return ErrorCode::BadData;
      }

      auto predecessorPorts = std::dynamic_pointer_cast<IPortConnector>(predecessor);
      if (!predecessorPorts)
      {
        LOG(ERROR) << "Predecessor declares no output port: " << predecessor->GetName();
        return ErrorCode::BadData;
      }

      // Port numbers start from 1 in the settings file
      const std::size_t portIndex = static_cast<std::size_t>(portNumber - 1);

      if (ports->SetInput(portIndex, predecessorPorts->GetOutput()) != ErrorCode::OK)
      {
        LOG(ERROR) << "Connection cannot be created: " << predecessor->GetName() << " -> "
                   << iModule->GetName() << " on port " << portNumber;
        return ErrorCode::BadData;
      }

      ss << "[" << predecessor->GetName() << ":" << portNumber << "]\t";
    }

    LOG(INFO) << ss.str();

    return ports->Connect();
  }
}
