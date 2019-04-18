#include "FaceApi.h"
#include "Framework/Profiler.h"
#include "Framework/Functional.hpp"

#include "Modules/ImageQueue/ImageQueue.h"
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
    mOutputQueue("OutputQueue", 100.0F, 10),
    mGraph(std::make_shared<Graph>())
  {
    START_EASYLOGGINGPP(0, static_cast<char**>(nullptr));
    mGraph->sFrameProcessed += MAKE_DELEGATE(&FaceApi::OnFrameProcessed, this);
  }

  FaceApi::~FaceApi()
  {
    DeInitialize();
    mGraph->sFrameProcessed -= MAKE_DELEGATE(&FaceApi::OnFrameProcessed, this);
  }

  fw::ErrorCode FaceApi::InitializeInternal(const cv::FileNode& /*iSettingsNode*/)
  {
    fw::ErrorCode result = fw::ErrorCode::OK;

    if ((result = Configuration::GetInstance().Initialize()) != fw::ErrorCode::OK)
      return result;

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
    mOutputQueue.Clear();

    return fw::ErrorCode::OK;
  }

  void FaceApi::Clear()
  {
    std::lock_guard<std::recursive_mutex> lock(sAppMutex);
    for (auto& module : mModules)
      module->Clear();

    mOutputQueue.Clear();
  }

  void FaceApi::OnFrameProcessed(ImageMessage::Shared iMessage)
  {
    if (!iMessage || iMessage->IsEmpty()) return;
    mOutputQueue.Push(iMessage);
  }

  // TODO-BEGIN: will be deleted
  fw::ErrorCode FaceApi::CreateModules()
  {
    if (!mFirstModule)		mModules.push_back(mFirstModule = new FirstModule());
    if (!mLastModule)		  mModules.push_back(mLastModule = new LastModule());
    if (!mImageQueue)		  mModules.push_back(mImageQueue = new ImageQueue());
    if (!mFaceDetection)	mModules.push_back(mFaceDetection = new FaceDetection());
    if (!mUserManager)		mModules.push_back(mUserManager = new UserManager());
    if (!mUserProcessor)	mModules.push_back(mUserProcessor = new UserProcessor());
    if (!mUserHistory)		mModules.push_back(mUserHistory = new UserHistory());
    if (!mVisualizer)		  mModules.push_back(mVisualizer = new Visualizer());

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
    mFirstModule->Connect();

    // Image queue: popping out a frame
    mImageQueue->SetInputPort<0>(mFirstModule->GetOutputPort());
    mImageQueue->Connect();

    // Face detector: detect faces on the frame pop from the queue
    mFaceDetection->SetInputPort<0>(mImageQueue->GetOutputPort());
    mFaceDetection->Connect();

    // User manager: manage active and inactive users, update them with the detected faces
    mUserManager->SetInputPort<0>(mImageQueue->GetOutputPort());
    mUserManager->SetInputPort<1>(mFaceDetection->GetOutputPort());
    mUserManager->Connect();

    // User processor: extract all of the features from faces (e.g. shape model, head pose)
    mUserProcessor->SetInputPort<0>(mImageQueue->GetOutputPort());
    mUserProcessor->SetInputPort<1>(mUserManager->GetOutputPort());
    mUserProcessor->Connect();

    // User history: maintain all entries that have been estimated by the user processor module in time
    mUserHistory->SetInputPort<0>(mUserProcessor->GetOutputPort());
    mUserHistory->Connect();

    // Visualizer: generate and draw the results
    mVisualizer->SetInputPort<0>(mImageQueue->GetOutputPort());
    mVisualizer->SetInputPort<1>(mUserProcessor->GetOutputPort());
    mVisualizer->Connect();

    // Last node: waiting for the results
    mLastModule->SetInputPort<0>(mVisualizer->GetOutputPort());
    mLastModule->Connect();

    return fw::ErrorCode::OK;
  }
  // TODO-END: will be deleted

  fw::ErrorCode FaceApi::PushCameraFrame(const cv::Mat& iFrame)
  {
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
      mGraph->Process();

      // TODO-BEGIN: will be deleted
      // Popping a frame out and waiting until the flow has been ended
      mFirstModule->Tick();
      mLastModule->Wait();

      // Pushing a debug frame if we have it
      if (mLastModule->HasOutput())
      {
        mOutputQueue.Push(mLastModule->GetLastImage());
      }
      // TODO-END: will be deleted

      ThreadSleep(1);
    }

    const std::string& profilerPath = Configuration::GetInstance().GetDirectories().output +
      "profiler." + fw::get_log_stamp() + ".txt";
    FACE_PROFILER_SAVE(profilerPath);

    return fw::ErrorCode::OK;
  }
}
