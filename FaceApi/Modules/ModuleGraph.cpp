#include "Framework/ErrorCode.h"
#include "Modules/ModuleGraph.h"

#include "Configuration.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include "Modules/ModuleFactory.h"
#include "Modules/ModuleConnector.h"

#include <easyloggingpp/easyloggingpp.h>
#include <queue>

namespace face
{
  // Upper bound for one frame, so a stalled graph cannot block the worker forever.
  const long long ModuleGraph::sProcessTimeoutMs = 5000LL;

  ModuleGraph::~ModuleGraph()
  {
    DeInitialize();
  }

  fw::ErrorCode ModuleGraph::Process()
  {
    if (!IsInitialized()) return fw::ErrorCode::BadState;

    DrainCommands();

    FACE_PROFILER_FRAME_ID(GetLastFrameId());

    // One frame is one unit of failure: OpenCV reports what it dislikes by throwing, and the
    // worker has no handler above it, so an escaping exception would end the process.
    try
    {
      // Reading the generation before the tick is what makes this a per-frame barrier.
      const unsigned long long generation = mLastModule->GetGeneration();

      mFirstModule->Tick();

      if (!mLastModule->WaitForNewOutput(generation, sProcessTimeoutMs))
      {
        LOG(WARNING) << "The module graph did not finish the frame within " << sProcessTimeoutMs << " ms.";
        return fw::ErrorCode::SystemFailure;
      }

      // Pushing a debug frame if we have it
      if (mLastModule->HasOutput())
      {
        mFrameProcessed.Raise(mLastModule->GetLastImage());
      }
    }
    catch (const cv::Exception& iException)
    {
      LOG(ERROR) << "Dropping the frame, OpenCV failed inside the module graph: " << iException.what();
      return fw::ErrorCode::SystemFailure;
    }
    catch (const std::exception& iException)
    {
      LOG(ERROR) << "Dropping the frame, a module failed: " << iException.what();
      return fw::ErrorCode::SystemFailure;
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

    // The aliases below are only ever assigned when empty, so drop the ones a previous
    // Initialize() left behind - otherwise this graph would tick the old graph's modules.
    // Done here and not in DeInitialize(): they are read from the app thread, and keeping
    // every write inside Initialize() keeps those reads safe.
    mModules.clear();
    mFirstModule = nullptr;
    mLastModule = nullptr;
    mImageQueue = nullptr;

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      if (moduleNode.empty() || !moduleNode.isNamed()) continue;

      // Check for duplications
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const std::shared_ptr<fw::Module>& obj) {
        return obj->GetName() == fw::Module::CreateModuleName(moduleNode);
      });

      if (it != mModules.end())
      {
        LOG(ERROR) << "Module is already defined: " << (*it)->GetName();
        return fw::ErrorCode::BadData;
      }

      // Extend this function if you add a new module
      auto newModule = ModuleFactory::Create(moduleNode, *mBus);

      // Check if the module is not set up in this file
      if (!newModule)
      {
        return fw::ErrorCode::BadData;
      }

      // Create an alias for the first and last module of the process
      // Duplications are already checked above
      if (!mFirstModule) mFirstModule = std::dynamic_pointer_cast<FirstModule>(newModule);

      if (!mLastModule) mLastModule = std::dynamic_pointer_cast<LastModule>(newModule);

      if (!mImageQueue) mImageQueue = std::dynamic_pointer_cast<ImageQueue>(newModule);

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

    if (!mImageQueue)
    {
      LOG(ERROR) << "Image queue module is not defined.";
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
      auto it = std::find_if(mModules.begin(), mModules.end(), [&](const std::shared_ptr<fw::Module>& obj) {
        return obj->GetName() == fw::Module::CreateModuleName(moduleNode);
      });

      // Unknown module
      if (it == mModules.end())
      {
        LOG(ERROR) << "Unknown module is referenced with name: " << moduleNode.name();
        return fw::ErrorCode::BadData;
      }

      std::shared_ptr<fw::Module> module = *it;
      PredecessorMap predecessors; // Key: port, value: module

      // Read the <port> tag of each module
      if ((result = GetPredecessors(moduleNode, iModulesNode, predecessors)) != fw::ErrorCode::OK)
      {
        return result;
      }

      // Source modules have no predecessor but still need their output port built
      if ((result = ModuleConnector::Connect(module, predecessors)) != fw::ErrorCode::OK)
      {
        return result;
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

        auto it = std::find_if(modulesPrio.begin(), modulesPrio.end(), [&](const ModulesPrioElem& iObj) {
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

    // Modules that are equally deep in the graph tie here, and the connection order
    // decides the order they are notified in. std::sort would break such ties
    // arbitrarily, so keep the order of the settings file instead.
    std::stable_sort(modulesPrio.begin(), modulesPrio.end(), [&](const ModulesPrioElem& iFirst, const ModulesPrioElem& iSecond) {
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
