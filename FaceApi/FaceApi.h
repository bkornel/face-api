#pragma once

#include "Common/Configuration.h"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"
#include "Graph/Graph.h"
#include "Modules/ImageQueue/ImageQueue.h"

#include <string>
#include <vector>

namespace face
{
	class FirstModule;
	class LastModule;
	class FaceDetection;
	class UserHistory;
	class UserManager;
	class UserProcessor;
	class Visualizer;

	class FaceApi :
		public fw::Module
	{
		typedef fw::MessageQueue<ImageMessage::Shared> MessageQueue;

	public:
		static FaceApi& GetInstance();

		virtual ~FaceApi();

		fw::ErrorCode PushCameraFrame(const cv::Mat& iFrame);

		fw::ErrorCode GetResultImage(cv::Mat& oResultImage);

		void Clear() override;

		void SetRunDetectionFlag(bool iForce = false);

		inline void OnOffVerbose()
		{
			static bool verbose = Configuration::GetInstance().GetOutput().verbose;
			verbose = !verbose;
			Configuration::GetInstance().SetVerboseMode(verbose);
		}

		inline unsigned GetLastFrameId() const
		{
			return mImageQueue->GetLastFrameId();
		}

		inline long long GetLastTimestamp() const
		{
			return mImageQueue->GetLastTimestamp();
		}

		void SetWorkingDirectory(const std::string& iWorkingDirectory)
		{
			Configuration::GetInstance().SetWorkingDirectory(iWorkingDirectory);
		}

	private:
		static std::recursive_mutex sAppMutex;		///< The mutex to lock critical sections

		FaceApi();

		FaceApi(const FaceApi& iOther) = delete;

		FaceApi& operator=(const FaceApi& iOther) = delete;

		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

		fw::ErrorCode DeInitializeInternal() override;

		fw::ErrorCode Run() override;

		fw::ErrorCode CreateModules();

		fw::ErrorCode CreateConnections();

		fw::Executor::Shared mExecutor = nullptr;

		std::vector<fw::Module*> mModules;
		std::shared_ptr<Graph> mGraph = nullptr;

		FirstModule* mFirstModule = nullptr;
		LastModule* mLastModule = nullptr;
		ImageQueue* mImageQueue = nullptr;
		FaceDetection* mFaceDetection = nullptr;
		UserHistory* mUserHistory = nullptr;
		UserManager* mUserManager = nullptr;
		UserProcessor* mUserProcessor = nullptr;
		Visualizer* mVisualizer = nullptr;

		MessageQueue mOutputQueue;
	};
}
