#include "Framework/Graph/ModuleGraph.h"

#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <queue>
#include <utility>

namespace fw
{
  void ModuleGraph::Clear()
  {
    for (auto& module : mModules)
      module->Clear();
  }

  ErrorCode ModuleGraph::InitializeInternal(const cv::FileNode& iModulesNode)
  {
    if (iModulesNode.empty())
    {
      return ErrorCode::NotFound;
    }

    ErrorCode result = ErrorCode::OK;

    // Create the modules and load their settings
    if ((result = CreateModules(iModulesNode)) != ErrorCode::OK)
    {
      return result;
    }

    // Connect modules to each other
    if ((result = CreateConnections(iModulesNode)) != ErrorCode::OK)
    {
      return result;
    }

    return ErrorCode::OK;
  }

  ErrorCode ModuleGraph::DeInitializeInternal()
  {
    for (auto& module : mModules)
    {
      module->DeInitialize();
    }

    mModules.clear();

    return ErrorCode::OK;
  }

  void ModuleGraph::OnModuleCreated(const std::shared_ptr<Module>& /*iModule*/)
  {
  }

  ErrorCode ModuleGraph::ValidateModules()
  {
    return ErrorCode::OK;
  }

  ErrorCode ModuleGraph::CreateModules(const cv::FileNode& iModulesNode)
  {
    assert(!iModulesNode.empty());

    // Drop what a previous Initialize() left behind, otherwise this graph would tick the
    // old graph's modules
    mModules.clear();

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      if (moduleNode.empty() || !moduleNode.isNamed()) continue;

      const std::string moduleName = Module::CreateModuleName(moduleNode);

      // Check for duplications
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const std::shared_ptr<Module>& obj) {
        return obj->GetName() == moduleName;
      });

      if (it != mModules.end())
      {
        LOG(ERROR) << "Module is already defined: " << (*it)->GetName();
        return ErrorCode::BadData;
      }

      auto newModule = CreateModule(moduleNode);

      // Check if the module is not set up in the derived factory
      if (!newModule)
      {
        return ErrorCode::BadData;
      }

      OnModuleCreated(newModule);

      LOG(INFO) << "New module is created: [" << newModule->GetName() << "]";

      mModules.emplace_back(newModule);
    }

    return ValidateModules();
  }

  ErrorCode ModuleGraph::CreateConnections(const cv::FileNode& iModulesNode)
  {
    assert(!iModulesNode.empty());

    ErrorCode result = ErrorCode::OK;
    std::vector<cv::FileNode> modules = GetConnectionOrder(iModulesNode);

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : modules)
    {
      const std::string moduleName = Module::CreateModuleName(moduleNode);

      // Find the corresponding module
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const std::shared_ptr<Module>& obj) {
        return obj->GetName() == moduleName;
      });

      // Unknown module
      if (it == mModules.end())
      {
        LOG(ERROR) << "Unknown module is referenced with name: " << moduleNode.name();
        return ErrorCode::BadData;
      }

      std::shared_ptr<Module> module = *it;
      PredecessorMap predecessors; // Key: port, value: module

      // Read the <port> tag of each module
      if ((result = GetPredecessors(moduleNode, predecessors)) != ErrorCode::OK)
      {
        return result;
      }

      // Source modules have no predecessor but still need their output port built
      if ((result = ModuleConnector::Connect(module, predecessors)) != ErrorCode::OK)
      {
        return result;
      }
    }

    return result;
  }

  ErrorCode ModuleGraph::GetPredecessors(const cv::FileNode& iModule, PredecessorMap& oPredecessors)
  {
    assert(!iModule.empty());

    // Collect predecessor modules of iModule
    oPredecessors.clear();

    // Check if it does not have a predecessor
    const cv::FileNode& portList = iModule["port"];
    if (portList.empty())
    {
      return ErrorCode::OK;
    }

    const std::string& moduleName = Module::CreateModuleName(iModule);

    // Loop over the <port> list of iModule
    for (const auto& portNode : portList)
    {
      const std::string& portNodeStr = str::trim(portNode.string());
      if (portNodeStr.empty())
      {
        LOG(ERROR) << "Input port is not specified. Module " << moduleName;
        return ErrorCode::NotFound;
      }

      // Tokenize the string: "predecessorName:portNumber"
      const auto tokens = str::split(portNodeStr, ':');
      if (tokens.size() != 2U)
      {
        LOG(ERROR) << "Input port format is wrong. Module " << moduleName << ", port: " << portNodeStr;
        return ErrorCode::BadData;
      }

      // Check the port number, indexing start from 1
      const int portNumber = str::convert_to_number<int>(str::trim(tokens[1]));
      if (portNumber < 1)
      {
        LOG(ERROR) << "Port number is less than 1. Module " << moduleName << ", port: " << portNodeStr;
        return ErrorCode::BadData;
      }

      // Find the predecessor between all of the modules
      const std::string& predecessorName = str::trim(tokens[0]);
      bool isFound = false;

      // Loop over the modules
      for (const auto& module : mModules)
      {
        if (predecessorName == module->GetName())
        {
          // If the port number is still not reserved
          if (oPredecessors[portNumber] == nullptr)
          {
            oPredecessors[portNumber] = module;
            isFound = true;
            break;
          }
          else
          {
            LOG(ERROR) << "Port number is already set. Module " << moduleName << ", port: " << portNodeStr;
            return ErrorCode::BadData;
          }
        }
      }

      // Predecessor could not be found in the settings file
      if (!isFound)
      {
        LOG(ERROR) << "Predecessor is not defined in the configuration file. Module " << moduleName << ", port: " << portNodeStr;
        return ErrorCode::BadData;
      }
    }

    return ErrorCode::OK;
  }

  std::vector<cv::FileNode> ModuleGraph::GetConnectionOrder(const cv::FileNode& iModulesNode)
  {
    assert(!iModulesNode.empty());

    // The name is kept alongside the node: it is what every lookup below matches on, and
    // rebuilding it per candidate meant composing the same string over and over.
    struct ModulesPrioElem
    {
      cv::FileNode node;
      std::string name;
      uint32_t depth = 0U;
    };

    std::vector<ModulesPrioElem> modulesPrio;

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      modulesPrio.emplace_back(ModulesPrioElem{ moduleNode, Module::CreateModuleName(moduleNode), 0U });
    }

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      std::queue<std::string> predecessors;

      // Loop over the <port> list of moduleNode
      for (const auto& portNode : moduleNode["port"])
      {
        // Tokenize the string: "predecessorName:portNumber"
        const auto tokens = str::split(str::trim(portNode.string()), ':');
        if (tokens.size() != 2U)
        {
          continue;
        }

        predecessors.emplace(str::trim(tokens[0]));
      }

      // Loop over the path of predecessors until the first module is not reached
      while (!predecessors.empty())
      {
        const std::string& predecessorName = predecessors.front();

        auto it = std::find_if(modulesPrio.begin(), modulesPrio.end(), [&](const ModulesPrioElem& iObj) {
          return predecessorName == iObj.name;
        });

        if (it != modulesPrio.end())
        {
          it->depth++;

          // Loop over the <port> list of the predecessor and queueing them
          for (const auto& portNode : it->node["port"])
          {
            // Tokenize the string: "predecessorName:portNumber"
            const auto tokens = str::split(str::trim(portNode.string()), ':');
            if (tokens.size() != 2U)
            {
              continue;
            }

            predecessors.emplace(str::trim(tokens[0]));
          }
        }

        predecessors.pop();
      }
    }

    // Modules that are equally deep in the graph tie here, and the connection order
    // decides the order they are notified in. std::sort would break such ties
    // arbitrarily, so keep the order of the settings file instead.
    std::stable_sort(modulesPrio.begin(), modulesPrio.end(), [](const ModulesPrioElem& iFirst, const ModulesPrioElem& iSecond) {
      return iFirst.depth > iSecond.depth;
    });

    std::vector<cv::FileNode> modules;
    modules.reserve(modulesPrio.size());

    for (const auto& m : modulesPrio)
    {
      modules.emplace_back(m.node);
    }

    return modules;
  }
}
