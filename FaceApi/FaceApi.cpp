#include <opencv2/highgui/highgui.hpp>

#include "FaceApi.h"
#include "Framework/Profiler.h"
#include "Framework/Functional.hpp"

#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"


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
		fw::Module("FaceApi"),
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

	fw::ErrorCode FaceApi::ThreadProcedure()
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
			mFirstNode.Tick();
			mLastNode.Wait();

			// Pushing a debug frame if we have it
			if (mLastNode.Get() && mVisualizer->HasOutput())
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
