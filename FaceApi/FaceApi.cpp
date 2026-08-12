#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include "FaceApi.h"

#include "Configuration.h"
#include "Framework/Profiler.h"
#include "Messages/CommandMessage.h"

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
    mModuleGraph(std::make_shared<ModuleGraph>())
  {
    START_EASYLOGGINGPP(0, static_cast<char**>(nullptr));

    // The singleton is not owned by a shared_ptr, so it gets an untracked subscription and
    // unsubscribes in DeInitialize(). The bus is a member, it outlives the modules.
    Attach(mBus, nullptr);
    mModuleGraph->Attach(mBus, mModuleGraph);

    mFrameProcessedToken = mModuleGraph->SubscribeFrameProcessed(
      [this](std::shared_ptr<ImageMessage> iMessage) { OnFrameProcessed(iMessage); });
  }

  FaceApi::~FaceApi()
  {
    DeInitialize();
    mModuleGraph->UnsubscribeFrameProcessed(mFrameProcessedToken);
  }

  fw::ErrorCode FaceApi::InitializeInternal(const cv::FileNode& /*iSettingsNode*/)
  {
    fw::ErrorCode result = fw::ErrorCode::OK;

    if ((result = Configuration::GetInstance().Initialize()) != fw::ErrorCode::OK) return result;

    // Create and init modules here
    if ((result = mModuleGraph->Initialize(Configuration::GetInstance().GetModulesNode())) != fw::ErrorCode::OK)
      return result;

    // Start worker threads
    if ((result = StartThread()) != fw::ErrorCode::OK) return result;

    if (Configuration::GetInstance().GetVerbose()) OnOffVerbose();

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode FaceApi::DeInitializeInternal()
  {
    // Before Clear(): Run() works on the graph this is about to reset
    StopThread();

    Clear();

    return fw::ErrorCode::OK;
  }

  void FaceApi::Clear()
  {
    std::lock_guard<std::recursive_mutex> lock(sAppMutex);
    mModuleGraph->Clear();
    mOutputQueue.Clear();
    mCameraFrameId = 0U;
  }

  void FaceApi::OnFrameProcessed(std::shared_ptr<ImageMessage> iMessage)
  {
    if (!iMessage || iMessage->IsEmpty()) return;

    if (mOutputQueue.TryPush(iMessage) == fw::ErrorCode::OutOfResources)
    {
      LOG(DEBUG) << "Output queue is full, dropping the processed frame.";
    }
  }

  void FaceApi::PushCameraFrame(const cv::Mat& iFrame)
  {
    std::shared_ptr<ImageQueue> imageQueue = mModuleGraph ? mModuleGraph->GetImageQueue() : nullptr;
    if (!imageQueue) return;

    // Handed straight to the queue. Broadcasting it would call every module for every
    // frame, and all but one of them only to find out they are not interested.
    imageQueue->Push(iFrame, mCameraFrameId++, fw::now());
  }

  fw::ErrorCode FaceApi::GetResults(FaceResults& oResults) const
  {
    oResults.clear();

    std::shared_ptr<LastModule> lastModule = mModuleGraph ? mModuleGraph->GetLastModule() : nullptr;
    if (!lastModule) return fw::ErrorCode::BadState;

    return lastModule->GetLastResults(oResults);
  }

  fw::ErrorCode FaceApi::GetResultImage(cv::Mat& oResultImage)
  {
    std::tuple<std::shared_ptr<ImageMessage>> framePool;
    const fw::ErrorCode code = mOutputQueue.TryPop(framePool);

    if (code != fw::ErrorCode::OK)
    {
      return code;
    }

    std::shared_ptr<ImageMessage> frame = std::get<0>(framePool);
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
      DrainCommands();
      FACE_PROFILER_FRAME_ID(GetLastFrameId());
      mModuleGraph->Process();

      ThreadSleep(1);
    }

    const std::string& profilerPath =
      Configuration::GetInstance().GetDirectories().output + "profiler." + fw::get_log_stamp() + ".txt";
    FACE_PROFILER_SAVE(profilerPath);
    FACE_PROFILER_SUMMARY();

    return fw::ErrorCode::OK;
  }

  void FaceApi::SetRunFaceDetector()
  {
    const fw::Timestamp timestamp = fw::now();
    Publish(
      std::make_shared<CommandMessage>(CommandMessage::Type::RunFaceDetection, mCameraFrameId, timestamp)
    );
  }

  void FaceApi::OnOffVerbose()
  {
    const fw::Timestamp timestamp = fw::now();
    Publish(
      std::make_shared<CommandMessage>(CommandMessage::Type::VerboseModeChanged, mCameraFrameId, timestamp)
    );
  }

  void FaceApi::SetWorkingDirectory(const std::string& iWorkingDirectory)
  {
    Configuration::GetInstance().SetWorkingDirectory(iWorkingDirectory);
  }
} // namespace face
