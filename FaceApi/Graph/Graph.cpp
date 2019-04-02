#include "Graph/Graph.h"

#include "3rdparty/easyloggingpp/easyloggingpp.h"
#include "Common/Configuration.h"
#include "Framework/UtilString.h"

#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"


namespace face
{
	Graph::Graph() :
		fw::Module("Graph"),
		mExecutor(fw::getInlineExecutor())
	{
	}

	Graph::~Graph()
	{
		DeInitialize();
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

		return result;
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
			std::shared_ptr<fw::Module> module = nullptr;

			if (moduleName == "imagequeue")			module = std::make_shared<ImageQueue>();
			else if (moduleName == "facedetection")	module = std::make_shared<FaceDetection>();
			else if (moduleName == "userhistory")	module = std::make_shared<UserHistory>();
			else if (moduleName == "usermanager")	module = std::make_shared<UserManager>();
			else if (moduleName == "userprocessor")	module = std::make_shared<UserProcessor>();
			else if (moduleName == "visualizer")	module = std::make_shared<Visualizer>();
			else
			{
				LOG(ERROR) << "Unknown module is referenced with name: " << moduleNode.name();
				return fw::ErrorCode::BadData;
			}

			fw::ErrorCode result = fw::ErrorCode::OK;
			if ((result = module->Initialize(moduleNode)) != fw::ErrorCode::OK)
			{
				return result;
			}

			CV_Assert(module != nullptr);

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
			ConnectionMap connectionMap;

			if ((result = GetPredecessors(moduleName, iModulesNode, connectionMap)) != fw::ErrorCode::OK)
			{
				return result;
			}
			
			if (!connectionMap.empty())
			{
				// TODO(kbertok)
			}
		}

		return result;
	}

	fw::ErrorCode Graph::GetPredecessors(const std::string& iModuleName, const cv::FileNode& iModulesNode, ConnectionMap& oConnectionMap)
	{
		CV_Assert(!iModuleName.empty() && !iModulesNode.empty());

		oConnectionMap.clear();

		if (iModuleName == "imagequeue")
		{
			oConnectionMap[1] = "first";
			return fw::ErrorCode::OK;
		}

		for (const auto& predecessorNode : iModulesNode)
		{
			const std::string& predecessorName = fw::str::to_lower(predecessorNode.name());
			if (iModuleName == predecessorName)
			{
				continue;
			}

			const cv::FileNode& connectionNodes = predecessorNode["connection"];
			if (connectionNodes.empty() || connectionNodes.size() == 0U)
			{
				continue;
			}

			for (const auto& connectionNode : connectionNodes)
			{
				const std::string& connection = fw::str::trim(fw::str::to_lower(connectionNode.string()));

				if (connection.empty())
				{
					continue;
				}

				const auto tokens = fw::str::split(connection, ':');

				if (tokens.size() != 2)
				{
					LOG(ERROR) << "Wrong format in connection: " << connectionNode.string();
					return fw::ErrorCode::BadData;
				}

				const int portNumber = fw::str::convert_to_number<int>(fw::str::trim(tokens[1]));
				if (portNumber < 1)
				{
					LOG(ERROR) << "Port number is less than 1: " << connectionNode.string();
					return fw::ErrorCode::BadData;
				}

				const std::string& connectionName = fw::str::trim(tokens[0]);

				if (connectionName == iModuleName)
				{
					if (oConnectionMap.find(portNumber) != oConnectionMap.end())
					{
						LOG(ERROR) << "Port(" << portNumber << ") is already used: " << iModuleName;
						return fw::ErrorCode::BadData;
					}

					oConnectionMap[portNumber] = predecessorName;
				}
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
