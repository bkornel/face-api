#include "Graph/Graph.h"

#include "Common/Configuration.h"
#include "Framework/Profiler.h"
#include "Framework/UtilString.h"

#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  Graph::FrameProcessedHandler Graph::sFrameProcessed;

  namespace
  {
    fw::Module::Shared create_module(const std::string& iModuleName)
    {
      const std::string& moduleName = fw::str::to_lower(iModuleName);

      if (moduleName == "imagequeue")			    return std::make_shared<ImageQueue>();
      else if (moduleName == "facedetection")	return std::make_shared<FaceDetection>();
      else if (moduleName == "firstmodule")		return std::make_shared<FirstModule>();
      else if (moduleName == "lastmodule")		return std::make_shared<LastModule>();
      else if (moduleName == "userhistory")		return std::make_shared<UserHistory>();
      else if (moduleName == "usermanager")		return std::make_shared<UserManager>();
      else if (moduleName == "userprocessor")	return std::make_shared<UserProcessor>();
      else if (moduleName == "visualizer")		return std::make_shared<Visualizer>();

      return nullptr;
    }
  }

  Graph::~Graph()
  {
    DeInitialize();
  }

  fw::ErrorCode Graph::Process()
  {
    // TODO: still not initialized
    return fw::ErrorCode::OK;


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

  void Graph::Clear()
  {
    for (auto& module : mModules)
      module->Clear();
  }

  fw::ErrorCode Graph::InitializeInternal(const cv::FileNode& iModulesNode)
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

  fw::ErrorCode Graph::DeInitializeInternal()
  {
    for (auto& module : mModules)
    {
      module->DeInitialize();
    }

    mModules.clear();

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode Graph::CreateModules(const cv::FileNode& iModulesNode)
  {
    CV_Assert(!iModulesNode.empty());

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      if (moduleNode.empty() || !moduleNode.isNamed()) continue;

      // Check for duplications
      for (const auto& m : mModules)
      {
        if (m->GetName() == fw::Module::CreateModuleName(moduleNode))
        {
          LOG(ERROR) << "Module is already defined: " << m->GetName();
          return fw::ErrorCode::BadData;
        }
      }

      // Extend this function if you add a new module
      auto newModule = create_module(moduleNode.name());

      // Check if the module is not set up in this file
      if (!newModule)
      {
        LOG(ERROR) << "Unknown module is referenced with name: " << moduleNode.name();
        return fw::ErrorCode::BadData;
      }

      // Initialize the module (this will also load the module's settings)
      fw::ErrorCode result = fw::ErrorCode::OK;
      if ((result = newModule->Initialize(moduleNode)) != fw::ErrorCode::OK)
      {
        return result;
      }

      // Create an alias for the first and last module of the process
      if (!mFirstModule) mFirstModule = std::dynamic_pointer_cast<FirstModule>(newModule);

      if (!mLastModule) mLastModule = std::dynamic_pointer_cast<LastModule>(newModule);

      mModules.push_back(newModule);
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

  fw::ErrorCode Graph::CreateConnections(const cv::FileNode& iModulesNode)
  {
    CV_Assert(!iModulesNode.empty());

    fw::ErrorCode result = fw::ErrorCode::OK;

    // Loop over the <modules> tag in the settings file
    for (const auto& moduleNode : iModulesNode)
    {
      PredecessorMap predecessors; // Key: port, value: module

      // Read the <port> tag of each module
      if ((result = GetPredecessors(moduleNode, iModulesNode, predecessors)) != fw::ErrorCode::OK)
      {
        return result;
      }

      if (!predecessors.empty())
      {
        // TODO(kbertok)
        const std::string& moduleName = fw::Module::CreateModuleName(moduleNode);
      }
    }

    return result;
  }

  fw::ErrorCode Graph::GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors)
  {
    CV_Assert(!iModule.empty() && !iModules.empty());

    // Collect predecessor modules of iModule
    oPredecessors.clear();

    // Check if it does not have a predecessor
    const cv::FileNode& portList = iModule["port"];
    if (portList.empty() || portList.size() == 0U)
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
      if (tokens.size() != 2)
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

  /*fw::ErrorCode GeneralGraph::Connect()
  {
    // This the first node in the execution
    mFirstNode.port = fw::connect(
      FW_BIND(&fw::FirstNode<unsigned>::Main, &mFirstNode),
      mExecutor);

    // Image queue: popping out a frame
    mImageQueue->port = fw::connect(
      FW_BIND(&ImageQueue::Pop, mImageQueue),
      mFirstNode.port.second);

    // Face detector: detect faces on the frame pop from the queue
    mFaceDetection->port = fw::connect(
      FW_BIND(&FaceDetection::Detect, mFaceDetection),
      mImageQueue->port);

    // User manager: manage active and inactive users, update them with the detected faces
    mUserManager->port = fw::connect(
      FW_BIND(&UserManager::Process, mUserManager),
      mImageQueue->port, mFaceDetection->port);

    // User processor: extract all of the features from faces (e.g. shape model, head pose)
    mUserProcessor->port = fw::connect(
      FW_BIND(&UserProcessor::Process, mUserProcessor),
      mImageQueue->port, mUserManager->port);

    // User history: maintain all entries that have been estimated by the user processor module in time
    mUserHistory->port = fw::connect(
      FW_BIND(&UserHistory::Process, mUserHistory),
      mUserProcessor->port);

    // Visualizer: generate and draw the results
    mVisualizer->port = fw::connect(
      FW_BIND(&Visualizer::Draw, mVisualizer),
      mImageQueue->port, mUserProcessor->port);

    // Last node: waiting for the results
    mLastNode.port = fw::connect(
      FW_BIND(&fw::LastNode<bool>::Main, &mLastNode),
      mVisualizer->port);

    return fw::ErrorCode::OK;
  }*/
}
