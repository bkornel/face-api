#include "Modules/ModuleGraph.h"

#include "Common/Configuration.h"

#include "Framework/Profiler.h"
#include "Framework/UtilString.h"

#include "Modules/ModuleFactory.h"
#include "Modules/ModuleConnector.h"

#include <easyloggingpp/easyloggingpp.h>
#include <queue>

namespace face
{
  ModuleGraph::FrameProcessedHandler ModuleGraph::sFrameProcessed;

  ModuleGraph::~ModuleGraph()
  {
    DeInitialize();
  }

  fw::ErrorCode ModuleGraph::Process()
  {
    if (!IsInitialized()) return fw::ErrorCode::BadState;

    FACE_PROFILER_FRAME_ID(GetLastFrameId());

    // Popping a frame out and waiting until the flow has been ended
    mFirstModule->Tick();
    mLastModule->Wait();

    // Pushing a debug frame if we have it
    if (mLastModule->HasOutput())
    {
      sFrameProcessed.Raise(mLastModule->GetLastImage());
    }

    return fw::ErrorCode::OK;
  }

  void ModuleGraph::Clear()
  {
    for (auto& module : mModules)
      module->Clear();
  }

  fw::ErrorCode ModuleGraph::InitializeInternal(const cv::FileNode& iModulesNode)
  {
    if (iModulesNode.empty())
    {
      return fw::ErrorCode::NotFound;
    }

    fw::ErrorCode result = fw::ErrorCode::OK;

    // Create the modules and load their settings
    if ((result = CreateModules(iModulesNode)) != fw::ErrorCode::OK)
    {
      return result;
    }

    // Connect modules to each other
    if ((result = CreateConnections(iModulesNode)) != fw::ErrorCode::OK)
    {
      return result;
    }

    // Start worker threads
    if ((result = StartThread()) != fw::ErrorCode::OK)
    {
      return result;
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ModuleGraph::DeInitializeInternal()
  {
    for (auto& module : mModules)
    {
      module->DeInitialize();
    }

    mModules.clear();

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ModuleGraph::CreateModules(const cv::FileNode& iModulesNode)
  {
    CV_DbgAssert(!iModulesNode.empty());

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      if (moduleNode.empty() || !moduleNode.isNamed()) continue;

      // Check for duplications
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const fw::Module::Shared& obj) 
      {
        return obj->GetName() == fw::Module::CreateModuleName(moduleNode);
      });

      if (it != mModules.end())
      {
        LOG(ERROR) << "Module is already defined: " << (*it)->GetName();
        return fw::ErrorCode::BadData;
      }

      // Extend this function if you add a new module
      auto newModule = ModuleFactory::Create(moduleNode);

      // Check if the module is not set up in this file
      if (!newModule)
      {
        return fw::ErrorCode::BadData;
      }

      // Create an alias for the first and last module of the process
      // Duplications are already checked above
      if (!mFirstModule) mFirstModule = std::dynamic_pointer_cast<FirstModule>(newModule);

      if (!mLastModule) mLastModule = std::dynamic_pointer_cast<LastModule>(newModule);

      LOG(INFO) << "New module is created: [" << newModule->GetName() << "]";

      mModules.emplace_back(newModule);
    }

    // First-, and last modules are mandatory
    if (!mFirstModule)
    {
      LOG(ERROR) << "First module is not defined.";
      return fw::ErrorCode::BadData;
    }

    if (!mLastModule)
    {
      LOG(ERROR) << "Last module is not defined.";
      return fw::ErrorCode::BadData;
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ModuleGraph::CreateConnections(const cv::FileNode& iModulesNode)
  {
    CV_DbgAssert(!iModulesNode.empty());

    fw::ErrorCode result = fw::ErrorCode::OK;
    std::vector<cv::FileNode> modules = GetConnectionOrder(iModulesNode);

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : modules)
    {
      // Find the corresponding module
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const fw::Module::Shared& obj)
      {
        return obj->GetName() == fw::Module::CreateModuleName(moduleNode);
      });

      // Unknown module
      if (it == mModules.end())
      {
        LOG(ERROR) << "Unknown module is referenced with name: " << moduleNode.name();
        return fw::ErrorCode::BadData;
      }

      fw::Module::Shared module = *it;
      PredecessorMap predecessors; // Key: port, value: module

      // Read the <port> tag of each module
      if ((result = GetPredecessors(moduleNode, iModulesNode, predecessors)) != fw::ErrorCode::OK)
      {
        return result;
      }

      if (!predecessors.empty())
      {
        if ((result = ModuleConnector::Connect(module, predecessors)) != fw::ErrorCode::OK)
        {
          return result;
        }
      }
      else
      {
        LOG(INFO) << "Predecessors of [" << module->GetName() << "]:\t---";
      }
    }

    return result;
  }

  fw::ErrorCode ModuleGraph::GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors)
  {
    CV_DbgAssert(!iModule.empty() && !iModules.empty());

    // Collect predecessor modules of iModule
    oPredecessors.clear();

    // Check if it does not have a predecessor
    const cv::FileNode& portList = iModule["port"];
    if (portList.empty())
    {
      return fw::ErrorCode::OK;
    }

    const std::string& moduleName = fw::Module::CreateModuleName(iModule);

    // Loop over the <port> list of iModule
    for (const auto& portNode : portList)
    {
      const std::string& portNodeStr = fw::str::trim(portNode.string());
      if (portNodeStr.empty())
      {
        LOG(ERROR) << "Input port is not specified. Module " << moduleName;
        return fw::ErrorCode::NotFound;
      }

      // Tokenize the string: "predecessorName:portNumber"
      const auto tokens = fw::str::split(portNodeStr, ':');
      if (tokens.size() != 2U)
      {
        LOG(ERROR) << "Input port format is wrong. Module " << moduleName << ", port: " << portNodeStr;
        return fw::ErrorCode::BadData;
      }

      // Check the port number, indexing start from 1
      const int portNumber = fw::str::convert_to_number<int>(fw::str::trim(tokens[1]));
      if (portNumber < 1)
      {
        LOG(ERROR) << "Port number is less than 1. Module " << moduleName << ", port: " << portNodeStr;
        return fw::ErrorCode::BadData;
      }

      // Find the predecessor between all of the modules
      const std::string& predecessorName = fw::str::trim(tokens[0]);
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
            return fw::ErrorCode::BadData;
          }
        }
      }

      // Predecessor could not be found in the settings file
      if (!isFound)
      {
        LOG(ERROR) << "Predecessor is not defined in the configuration file. Module " << moduleName << ", port: " << portNodeStr;
        return fw::ErrorCode::BadData;
      }
    }

    return fw::ErrorCode::OK;
  }

  std::vector<cv::FileNode> ModuleGraph::GetConnectionOrder(const cv::FileNode& iModulesNode)
  {
    CV_DbgAssert(!iModulesNode.empty());

    using ModulesPrioElem = std::pair<cv::FileNode, unsigned>;
    std::vector<ModulesPrioElem> modulesPrio;

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      modulesPrio.emplace_back(moduleNode, 0U);
    }

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      std::queue<std::string> predecessors;

      // Loop over the <port> list of moduleNode
      for (const auto& portNode : moduleNode["port"])
      {
        // Tokenize the string: "predecessorName:portNumber"
        const auto tokens = fw::str::split(fw::str::trim(portNode.string()), ':');
        if (tokens.size() != 2U)
        {
          continue;
        }

        predecessors.emplace(fw::str::trim(tokens[0]));
      }

      // Loop over the path of predecessors until the first module is not reached
      while (!predecessors.empty())
      {
        const std::string& predecessorName = predecessors.front();

        auto it = std::find_if(modulesPrio.begin(), modulesPrio.end(), [&](const ModulesPrioElem& iObj)
        {
          return predecessorName == fw::Module::CreateModuleName(iObj.first);
        });

        if (it != modulesPrio.end()) 
        {
          it->second++;

          // Loop over the <port> list of it->first and queueing them
          for (const auto& portNode : it->first["port"])
          {
            // Tokenize the string: "predecessorName:portNumber"
            const auto tokens = fw::str::split(fw::str::trim(portNode.string()), ':');
            if (tokens.size() != 2U)
            {
              continue;
            }

            predecessors.emplace(fw::str::trim(tokens[0]));
          }
        }

        predecessors.pop();
      }
    }

    std::sort(modulesPrio.begin(), modulesPrio.end(), [&](const ModulesPrioElem& iFirst, const ModulesPrioElem& iSecond)
    {
      return iFirst.second > iSecond.second;
    });

    std::vector<cv::FileNode> modules;
    for (const auto& m : modulesPrio)
    {
      modules.emplace_back(m.first);
    }

    return modules;
  }
}
