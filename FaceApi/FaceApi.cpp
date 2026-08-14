#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include "FaceApi.h"

#include "Framework/Profiler.h"

INITIALIZE_EASYLOGGINGPP

namespace face
{
  namespace
  {
    /// @brief The longest one wait for a frame may last. Short enough that a stop request
    /// or a posted command is honoured promptly, long enough that an idle pipeline sleeps
    /// instead of polling.
    constexpr fw::Milliseconds sFrameWaitTimeout{ 50.0 };
  }

  FaceApi::FaceApi() :
    mOutputQueue("OutputQueue", 100.0F, 10),
    mPipeline(std::make_shared<FacePipeline>())
  {
    START_EASYLOGGINGPP(0, static_cast<char**>(nullptr));

    mFrameProcessedToken = mPipeline->SubscribeFrameProcessed(
      [this](std::shared_ptr<ImageMessage> iMessage) { OnFrameProcessed(iMessage); });
  }

  FaceApi::~FaceApi()
  {
    DeInitialize();

    mPipeline->UnsubscribeFrameProcessed(mFrameProcessedToken);
  }

  fw::ErrorCode FaceApi::InitializeInternal(const cv::FileNode& /*iSettingsNode*/)
  {
    fw::ErrorCode result = fw::ErrorCode::OK;

    // Read on every initialization, so a pipeline reload picks up a freshly saved file
    if ((result = mConfiguration.Initialize(GetWorkingDirectory())) != fw::ErrorCode::OK)
      return result;

    // Wired per initialization: DeInitialize() drops every subscription. Posted rather
    // than applied in place - Clear() takes the lock the graph runs under, and the signal
    // is raised from the graph thread.
    Listen(mPipeline->GetEvents().imageSizeChanged,
           [this](cv::Size /*iSize*/) { PostCommand([this] { Clear(); }); });

    // Create and init modules here
    mPipeline->SetWorkingDirectory(mConfiguration.GetDirectories().working);

    if ((result = mPipeline->Initialize(mConfiguration.GetModulesNode())) != fw::ErrorCode::OK)
      return result;

    // Start worker threads
    if ((result = StartThread()) != fw::ErrorCode::OK) return result;

    SetVerbose(mConfiguration.GetVerbose());

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode FaceApi::DeInitializeInternal()
  {
    // Before Clear(): Run() works on the graph this is about to reset
    StopThread();

    Clear();

    // Initialize() may run again with new settings; the modules must be built anew then
    return mPipeline->DeInitialize();
  }

  void FaceApi::Clear()
  {
    std::lock_guard<std::mutex> lock(mProcessMutex);
    mPipeline->Clear();
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

  fw::ErrorCode FaceApi::PushCameraFrame(const cv::Mat& iFrame)
  {
    std::shared_ptr<ImageQueue> imageQueue = mPipeline ? mPipeline->GetImageQueue() : nullptr;
    if (!imageQueue) return fw::ErrorCode::BadState;

    // Handed straight to the queue. Broadcasting it would call every module for every
    // frame, and all but one of them only to find out they are not interested.
    return imageQueue->Push(iFrame, mCameraFrameId++, fw::now());
  }

  fw::ErrorCode FaceApi::GetResults(FaceResults& oResults) const
  {
    oResults.clear();

    if (!mPipeline) return fw::ErrorCode::BadState;

    return mPipeline->GetLastResults(oResults);
  }

  fw::ErrorCode FaceApi::GetResultImage(cv::Mat& oResultImage)
  {
    std::shared_ptr<ImageMessage> frame;
    const fw::ErrorCode code = mOutputQueue.TryPop(frame);

    if (code != fw::ErrorCode::OK)
    {
      return code;
    }

    oResultImage = frame->GetFrameBGR();

    return code;
  }

  int FaceApi::GetQueueSize() const
  {
    std::shared_ptr<ImageQueue> imageQueue = mPipeline ? mPipeline->GetImageQueue() : nullptr;
    return imageQueue ? imageQueue->GetQueueStatistics().size : 0;
  }

  uint64_t FaceApi::GetDroppedFrameCount() const
  {
    std::shared_ptr<ImageQueue> imageQueue = mPipeline ? mPipeline->GetImageQueue() : nullptr;
    return imageQueue ? imageQueue->GetDroppedFrameCount() : 0ULL;
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

      // Outside the lock: a posted command may be the one that calls Clear(), which takes it
      DrainCommands();

      // Sleeps until a frame arrives instead of ticking on a timer: the timer used to run
      // every module a thousand times a second only to find out there was nothing to do
      std::shared_ptr<ImageQueue> imageQueue = mPipeline->GetImageQueue();
      if (!imageQueue || !imageQueue->WaitForFrame(sFrameWaitTimeout)) continue;

      {
        std::lock_guard<std::mutex> lock(mProcessMutex);
        FACE_PROFILER_FRAME_ID(GetLastFrameId());
        mPipeline->Process();
      }
    }

    const std::string& profilerPath =
      mConfiguration.GetDirectories().output + "profiler." + fw::get_log_stamp() + ".txt";
    FACE_PROFILER_SAVE(profilerPath);
    FACE_PROFILER_SUMMARY();

    return fw::ErrorCode::OK;
  }

  void FaceApi::SetRunFaceDetector()
  {
    if (mPipeline) mPipeline->GetEvents().runFaceDetection.Raise();
  }

  void FaceApi::SetVerbose(bool iVerbose)
  {
    // The signal carries the value rather than asking for a flip: a module that misses one
    // or handles it twice would otherwise be left inverted for the rest of the run.
    mVerbose = iVerbose;

    if (mPipeline) mPipeline->GetEvents().verboseChanged.Raise(iVerbose);
  }

  void FaceApi::OnOffVerbose()
  {
    SetVerbose(!mVerbose);
  }
} // namespace face
