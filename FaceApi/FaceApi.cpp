#include "FaceApi.h"
#include "Framework/Profiler.h"
#include "Framework/Functional.hpp"

#include "Modules/General/FirstModule.h"
#include "Modules/General/LastModule.h"
#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"

#include <opencv2/highgui/highgui.hpp>

INITIALIZE_EASYLOGGINGPP

namespace face
{
	std::recursive_mutex FaceApi::sAppMutex;

	FaceApi& FaceApi::GetInstance()
	{
		static FaceApi sInstance;
		return sInstance;
	}

	FaceApi::FaceApi() :
		mExecutor(fw::getInlineExecutor()),
		mOutputQueue("OutputQueue", 100.0F, 10)
	{
		START_EASYLOGGINGPP(0, static_cast<char**>(nullptr));
	}

	FaceApi::~FaceApi()
	{
		DeInitialize();
	}

	fw::ErrorCode FaceApi::InitializeInternal(const cv::FileNode& /*iSettingsNode*/)
	{
		fw::ErrorCode result = fw::ErrorCode::OK;

		if ((result = Configuration::GetInstance().Initialize()) != fw::ErrorCode::OK) 
			return result;

		mGraph = std::make_shared<Graph>();

		// Create and init modules here
		if ((result = mGraph->Initialize(Configuration::GetInstance().GetModulesNode())) != fw::ErrorCode::OK)
			return result;

		// Create and init modules here
		if ((result = CreateModules()) != fw::ErrorCode::OK) 
			return result;

		// Connect the modules with each other
		if ((result = CreateConnections()) != fw::ErrorCode::OK) 
			return result;

		// Start worker threads
		if ((result = StartThread()) != fw::ErrorCode::OK) 
			return result;

		return fw::ErrorCode::OK;
	}

	fw::ErrorCode FaceApi::DeInitializeInternal()
	{
		for (auto& module : mModules)
		{
			module->DeInitialize();
			delete module; module = nullptr;
		}

		mModules.clear();

		return fw::ErrorCode::OK;
	}

	void FaceApi::Clear()
	{
		std::lock_guard<std::recursive_mutex> lock(sAppMutex);
		for (auto& module : mModules)
			module->Clear();
	}

	fw::ErrorCode FaceApi::CreateModules()
	{
		if (!mFirstModule)		mModules.push_back(mFirstModule     = new FirstModule());
		if (!mLastModule)		mModules.push_back(mLastModule      = new LastModule());
		if (!mImageQueue)		mModules.push_back(mImageQueue		= new ImageQueue());
		if (!mFaceDetection)	mModules.push_back(mFaceDetection	= new FaceDetection());
		if (!mUserManager)		mModules.push_back(mUserManager		= new UserManager());
		if (!mUserProcessor)	mModules.push_back(mUserProcessor	= new UserProcessor());
		if (!mUserHistory)		mModules.push_back(mUserHistory		= new UserHistory());
		if (!mVisualizer)		mModules.push_back(mVisualizer		= new Visualizer());

		for (auto& m : mModules)
		{
			const cv::FileNode moduleSettings = Configuration::GetInstance().GetModuleSettings(m->GetName());
			const fw::ErrorCode result = m->Initialize(moduleSettings);
			if (result != fw::ErrorCode::OK)
				return result;
		}

		return fw::ErrorCode::OK;
	}

	fw::ErrorCode FaceApi::CreateConnections()
	{
		// This the first node in the execution
		mFirstModule->Connect(mExecutor);

		// Image queue: popping out a frame
		mImageQueue->SetArgument(mFirstModule->GetPort(), 0U);

		mImageQueue->mPort = fw::connect(
			FW_BIND(&ImageQueue::Main, mImageQueue),
			mFirstModule->GetPort());

		// Face detector: detect faces on the frame pop from the queue
		mFaceDetection->mPort = fw::connect(
			FW_BIND(&FaceDetection::Main, mFaceDetection),
			mImageQueue->GetPort());

		// User manager: manage active and inactive users, update them with the detected faces
		mUserManager->mPort = fw::connect(
			FW_BIND(&UserManager::Main, mUserManager),
			mImageQueue->GetPort(), mFaceDetection->GetPort());

		// User processor: extract all of the features from faces (e.g. shape model, head pose)
		mUserProcessor->mPort = fw::connect(
			FW_BIND(&UserProcessor::Main, mUserProcessor),
			mImageQueue->GetPort(), mUserManager->GetPort());

		// User history: maintain all entries that have been estimated by the user processor module in time
		mUserHistory->mPort = fw::connect(
			FW_BIND(&UserHistory::Main, mUserHistory),
			mUserProcessor->GetPort());

		// Visualizer: generate and draw the results
		mVisualizer->mPort = fw::connect(
			FW_BIND(&Visualizer::Main, mVisualizer),
			mImageQueue->GetPort(), mUserProcessor->GetPort());

		// Last node: waiting for the results
		mLastModule->mPort = fw::connect(
			FW_BIND(&LastModule::Main, mLastModule),
			mVisualizer->GetPort());

		return fw::ErrorCode::OK;
	}

	fw::ErrorCode FaceApi::PushCameraFrame(const cv::Mat& iFrame)
	{
		if (!iFrame.empty() && iFrame.size() != mImageQueue->GetImageSize())
		{
			LOG(INFO) << "Image size has been changed to: " << iFrame.size();
			Clear();

			// Set minimal face size for the user manager
			if (mFaceDetection && mUserManager)
			{
				const float minFaceSize = mFaceDetection->GetMinSize();
				if (minFaceSize > 0.0F && minFaceSize < 1.0F)
				{
					const int minSide = (std::min)(iFrame.rows, iFrame.cols);
					mUserManager->SetMinFaceSize({ 
						cvRound(minSide * minFaceSize), cvRound(minSide * minFaceSize)
					});
				}
			}
		}

		return mImageQueue->Push(iFrame);
	}

	fw::ErrorCode FaceApi::GetResultImage(cv::Mat& oResultImage)
	{
		std::tuple<ImageMessage::Shared> framePool;
		const fw::ErrorCode code = mOutputQueue.TryPop(framePool);

		if (code != fw::ErrorCode::OK)
		{
			return code;
		}

		ImageMessage::Shared frame = std::get<0>(framePool);
		oResultImage = frame->GetFrameBGR();

		return code;
	}

	fw::ErrorCode FaceApi::Run()
	{
		while (!GetThreadStopSignal())
		{
			if (!IsInitialized())
			{
				ThreadSleep(1);
				continue;
			}

			std::lock_guard<std::recursive_mutex> lock(sAppMutex);
			FACE_PROFILER_FRAME_ID(GetLastFrameId());

			// Decision about forcing the face detector on the current frame
			SetRunDetectionFlag();

			// Buffer visualization
			mVisualizer->SetQueueSize(mImageQueue->GetQueueSize());

			// Popping a frame out and waiting until the flow has been ended
			mFirstModule->Tick();
			mLastModule->Wait();

			// Pushing a debug frame if we have it
			if (mLastModule->Get() && mVisualizer->HasOutput())
			{
				auto outputMessage = std::make_shared<ImageMessage>(
					mVisualizer->GetResultImage(), 
					GetLastFrameId(), 
					fw::get_current_time()
				);
				mOutputQueue.Push(outputMessage);
			}

			ThreadSleep(1);
		}

		const std::string& profilerPath = Configuration::GetInstance().GetDirectories().output +
			"profiler." + fw::get_log_stamp() + ".txt";
		FACE_PROFILER_SAVE(profilerPath);

		return fw::ErrorCode::OK;
	}

	void FaceApi::SetRunDetectionFlag(bool iForce /*= false*/)
	{
		if (!mFaceDetection || !mUserManager) return;

		const bool shouldRun = (iForce || !mUserManager->IsAllPossibleUsersTracked());

		mFaceDetection->SetForceRun(shouldRun);
	}
}
