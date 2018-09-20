#pragma once

#include <string>
#include <vector>

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"
#include "Modules/ImageQueue/ImageQueue.h"

namespace face
{
	class FaceDetection;
	class UserHistory;
	class UserManager;
	class UserProcessor;
	class Visualizer;

	class FaceApp :
		public fw::Module
	{
		typedef fw::MessageQueue<ImageMessage::Shared> MessageQueue;

	public:
		static FaceApp& GetInstance();

		virtual ~FaceApp();

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

		FaceApp();

		FaceApp(const FaceApp& iOther) = delete;

		FaceApp& operator=(const FaceApp& iOther) = delete;

		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

		fw::ErrorCode DeInitializeInternal() override;

		fw::ErrorCode ThreadProcedure() override;

		fw::ErrorCode CreateModules();

		fw::ErrorCode CreateConnections();

		fw::Executor::Shared mExecutor = nullptr;
		fw::FirstNode<unsigned> mFirstNode;
		fw::LastNode<bool> mLastNode;

		std::vector<fw::Module*> mModules;

		ImageQueue* mImageQueue = nullptr;
		FaceDetection* mFaceDetection = nullptr;
		UserHistory* mUserHistory = nullptr;
		UserManager* mUserManager = nullptr;
		UserProcessor* mUserProcessor = nullptr;
		Visualizer* mVisualizer = nullptr;

		MessageQueue mOutputQueue;
	};
}
