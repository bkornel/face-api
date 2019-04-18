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
    template<typename T>
    std::shared_ptr<T> create_module(const std::string& iModuleName)
    {
      if (iModuleName == "imagequeue")			return std::make_shared<ImageQueue>();
      else if (iModuleName == "facedetection")	return std::make_shared<FaceDetection>();
      else if (iModuleName == "firstmodule")		return std::make_shared<FirstModule>();
      else if (iModuleName == "lastmodule")		return std::make_shared<LastModule>();
      else if (iModuleName == "userhistory")		return std::make_shared<UserHistory>();
      else if (iModuleName == "usermanager")		return std::make_shared<UserManager>();
      else if (iModuleName == "userprocessor")	return std::make_shared<UserProcessor>();
      else if (iModuleName == "visualizer")		return std::make_shared<Visualizer>();

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
    if ((result = CreateModules(iModulesNode)) != fw::ErrorCode::OK)
    {
      return result;
    }

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

    for (const auto& moduleNode : iModulesNode)
    {
      if (moduleNode.empty() || !moduleNode.isNamed()) continue;

      const std::string& moduleName = fw::str::to_lower(moduleNode.name());
      auto module = create_module<fw::Module>(moduleName);

      if (!module)
      {
        LOG(ERROR) << "Unknown module is referenced with name: " << moduleName;
        return fw::ErrorCode::BadData;
      }

      fw::ErrorCode result = fw::ErrorCode::OK;
      if ((result = module->Initialize(moduleNode)) != fw::ErrorCode::OK)
      {
        return result;
      }

      mModules.push_back(module);
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode Graph::CreateConnections(const cv::FileNode& iModulesNode)
  {
    CV_Assert(!iModulesNode.empty());

    fw::ErrorCode result = fw::ErrorCode::OK;

    for (const auto& moduleNode : iModulesNode)
    {
      const std::string& moduleName = fw::str::to_lower(moduleNode.name());
      PredecessorMap predecessors;

      if ((result = GetPredecessors(moduleNode, iModulesNode, predecessors)) != fw::ErrorCode::OK)
      {
        return result;
      }

      if (!predecessors.empty())
      {
        // TODO(kbertok)
      }
    }

    return result;
  }

  fw::ErrorCode Graph::GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors)
  {
    CV_Assert(!iModule.empty() && !iModules.empty());

    oPredecessors.clear();

    // It does not have any predecessor
    const cv::FileNode& ports = iModule["port"];
    if (ports.empty() || ports.size() == 0U)
    {
      return fw::ErrorCode::OK;
    }

    const std::string& moduleName = fw::str::to_lower(iModule.name());

    for (const auto& portNode : ports)
    {
      const std::string& port = fw::str::trim(fw::str::to_lower(portNode.string()));
      if (port.empty())
      {
        LOG(ERROR) << "Port is not specified for: " << moduleName;
        return fw::ErrorCode::NotFound;
      }

      const auto tokens = fw::str::split(port, ':');
      if (tokens.size() != 2)
      {
        LOG(ERROR) << "Port format is wrong for: " << moduleName << ", port: " << port;
        return fw::ErrorCode::BadData;
      }

      const int portNumber = fw::str::convert_to_number<int>(fw::str::trim(tokens[1]));
      if (portNumber < 1)
      {
        LOG(ERROR) << "Port number is less than 1 for: " << moduleName << ", port: " << port;
        return fw::ErrorCode::BadData;
      }

      const std::string& portName = fw::str::trim(tokens[0]);
      bool isFound = false;

      for (const auto& module : iModules)
      {
        const std::string& predecessorName = fw::str::to_lower(module.name());
        if (portName == predecessorName)
        {
          if (oPredecessors[portNumber].empty())
          {
            oPredecessors[portNumber] = predecessorName;
            isFound = true;
            break;
          }
          else
          {
            LOG(ERROR) << "Port number is already set for: " << moduleName << ", port: " << port;
            return fw::ErrorCode::BadData;
          }
        }
      }

      if (!isFound)
      {
        LOG(ERROR) << "Port is not defined in the configuration file for: " << moduleName << ", port: " << port;
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
