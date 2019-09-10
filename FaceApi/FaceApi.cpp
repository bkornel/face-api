#include "FaceApi.h"

#include "Common/Configuration.h"
#include "Framework/Profiler.h"

#include "Messages/CommandMessage.h"
#include "Messages/ImageArrivedMessage.h"

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

    // Start worker threads
    if ((result = StartThread()) != fw::ErrorCode::OK)
      return result;

    if (Configuration::GetInstance().GetVerbose())
      OnOffVerbose();

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode FaceApi::DeInitializeInternal()
  {
    Clear();
    return fw::ErrorCode::OK;
  }

  void FaceApi::Clear()
  {
    std::lock_guard<std::recursive_mutex> lock(sAppMutex);
    mGraph->Clear();
    mOutputQueue.Clear();
    mCameraFrameId = 0U;
  }

  void FaceApi::OnFrameProcessed(ImageMessage::Shared iMessage)
  {
    if (!iMessage || iMessage->IsEmpty()) return;
    mOutputQueue.Push(iMessage);
  }

  void FaceApi::PushCameraFrame(const cv::Mat& iFrame)
  {
    const long long timestamp = fw::get_current_time();

    sCommand.Raise(std::make_shared<ImageArrivedMessage>(iFrame, mCameraFrameId++, timestamp));
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

      ThreadSleep(1);
    }

    const std::string& profilerPath = Configuration::GetInstance().GetDirectories().output +
      "profiler." + fw::get_log_stamp() + ".txt";
    FACE_PROFILER_SAVE(profilerPath);

    return fw::ErrorCode::OK;
  }

  void FaceApi::SetRunFaceDetector()
  {
    const long long timestamp = fw::get_current_time();
    sCommand.Raise(std::make_shared<CommandMessage>(CommandMessage::Type::RunFaceDetection, mCameraFrameId, timestamp));
  }

  void FaceApi::OnOffVerbose()
  {
    const long long timestamp = fw::get_current_time();
    sCommand.Raise(std::make_shared<CommandMessage>(CommandMessage::Type::VerboseModeChanged, mCameraFrameId, timestamp));
  }

  void FaceApi::SetWorkingDirectory(const std::string& iWorkingDirectory)
  {
    Configuration::GetInstance().SetWorkingDirectory(iWorkingDirectory);
  }
}
