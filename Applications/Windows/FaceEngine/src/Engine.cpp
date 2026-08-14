#include "Engine.h"

#include "CameraEnumerator.h"
#include "TextConversions.h"

#include "Configuration.h"
#include "Framework/MathExtensions.h"
#include "Framework/Profiler.h"
#include "Framework/Settings.h"
#include "Framework/TimeExtensions.h"

#include <opencv2/imgcodecs.hpp>

#include <windows.h>

#include <algorithm>
#include <chrono>

namespace fe
{
  namespace
  {
    int64_t NowMs()
    {
      return fw::to_epoch_ms(fw::now());
    }
  }

  Engine::Engine() :
    mCaptureRate(1000LL),
    mPipelineRate(1000LL),
    mRenderRate(1000LL),
    mLatency(240U)
  {
    mOverlayOptions.glowStrength = 1.0;
    mOverlayOptions.pointSize = 1.6;
    mOverlayOptions.showMesh = 1;
    mOverlayOptions.showPoints = 1;
    mOverlayOptions.showRect = 1;
    mOverlayOptions.showPoseBox = 1;
    mOverlayOptions.showAxes = 1;
    mOverlayOptions.showLabels = 1;
    mOverlayOptions.mirror = 1;

    mHeadOptions.orbitYawDeg = 0.0;
    mHeadOptions.orbitPitchDeg = 0.0;
    mHeadOptions.distance = 3.1;
    mHeadOptions.poseFollow = 1.0;
    mHeadOptions.expression = 1.2;
    mHeadOptions.showWireframe = 0;
    mHeadOptions.showLandmarks = 0;
    mHeadOptions.showEyes = 1;
    mHeadOptions.idleSpin = 1;
  }

  Engine::~Engine()
  {
    Shutdown();
  }

  bool Engine::Initialize(const std::wstring& iWorkingDirectory)
  {
    if (mInitialized) return true;

    mWorkingDirectory = ToUtf8(iWorkingDirectory);

    // Everything under the working directory is addressed by concatenation, so the
    // separator has to be there before the pipeline is told about it
    if (!mWorkingDirectory.empty() &&
        mWorkingDirectory.back() != '\\' && mWorkingDirectory.back() != '/')
    {
      mWorkingDirectory += '\\';
    }

    mFaceApi.SetWorkingDirectory(mWorkingDirectory);

    static const cv::FileNode sEmptyNode;

    if (mFaceApi.Initialize(sEmptyNode) != fw::ErrorCode::OK)
    {
      SetLastError(L"The pipeline could not be initialised from " + iWorkingDirectory +
                   L". Check that settings.json and the model files are there.");
      return false;
    }

    DetectPipelineOverlay();

    {
      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      if (!mDevice.Create())
      {
        SetLastError(L"No Direct3D 11 device could be created.");
        mFaceApi.DeInitialize();
        return false;
      }
    }

    mCapture = std::make_unique<CaptureSource>([this](const cv::Mat& iFrame) { OnCameraFrame(iFrame); });

    mRenderStop.store(false, std::memory_order_release);
    mRenderThread = std::thread(&Engine::RenderLoop, this);

    mInitialized = true;
    return true;
  }

  void Engine::Shutdown()
  {
    if (!mInitialized) return;

    mInitialized = false;

    // The source first: it feeds the pipeline, and stopping the pipeline under a running
    // capture would only make the queue overflow on the way out
    if (mCapture) mCapture->Close();

    mRenderStop.store(true, std::memory_order_release);
    if (mRenderThread.joinable()) mRenderThread.join();

    mFaceApi.DeInitialize();

    {
      std::lock_guard<std::mutex> lock(mRecordMutex);
      mVideoWriter.Close();
      mRecording.store(false, std::memory_order_release);
    }

    {
      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      mHeadView.reset();
      mVideoView.reset();
      mDevice.Destroy();
    }

    mCapture.reset();

    {
      std::lock_guard<std::mutex> lock(mFrameMutex);
      mLastFrame.reset();
    }
  }

  void Engine::SetLastError(const std::wstring& iMessage)
  {
    std::lock_guard<std::mutex> lock(mErrorMutex);
    mLastError = iMessage;
  }

  void Engine::DetectPipelineOverlay()
  {
    // The Visualizer draws the overlay into the frame itself. This application draws its own
    // on the GPU, so it has to know whether it would be drawing a second one over the first.
    // A visualizer in the graph is a visualizer whose frame comes out of the pipeline.
    const bool visualizerWired = !mFaceApi.GetConfiguration().GetModuleSettings("visualizer").empty();

    mPipelineDrawsOverlay.store(visualizerWired, std::memory_order_release);

    // The artificial head is deployed with the shape model, so the model's own directory
    // says where the head lives - a moved configurations directory moves both together
    std::string modelDir = "shapemodel/3ddfa/";
    fw::get_value(mFaceApi.GetConfiguration().GetModuleSettings("shapeModel"), "modelDir", modelDir);

    mHeadModelPath = mFaceApi.GetConfiguration().GetDirectories().working + modelDir + "../head/ict_neutral_head.obj";
  }

  int Engine::EnumerateCameras(int iMaxProbe)
  {
    mCameraNames.clear();

    for (const auto& device : CameraEnumerator::Enumerate())
    {
      mCameraNames.emplace_back(device.name);
    }

    // No DirectShow, or a machine whose devices it does not list. The indices are still
    // worth offering, because that is all cv::VideoCapture needs.
    if (mCameraNames.empty() && iMaxProbe > 0)
    {
      for (int i = 0; i < (std::min)(iMaxProbe, 8); ++i)
      {
        cv::VideoCapture probe;

        if (probe.open(i, cv::CAP_DSHOW))
        {
          mCameraNames.emplace_back(L"Camera " + std::to_wstring(i));
          probe.release();
        }
      }
    }

    return static_cast<int>(mCameraNames.size());
  }

  bool Engine::GetCameraName(int iIndex, std::wstring& oName) const
  {
    if (iIndex < 0 || iIndex >= static_cast<int>(mCameraNames.size())) return false;

    oName = mCameraNames[static_cast<std::size_t>(iIndex)];
    return true;
  }

  bool Engine::OpenCamera(int iIndex, int iWidth, int iHeight, double iFps)
  {
    if (!mCapture) return false;

    std::wstring error;

    if (!mCapture->OpenCamera(iIndex, iWidth, iHeight, iFps, error))
    {
      SetLastError(error.empty() ? L"The camera could not be opened." : error);
      return false;
    }

    ClearUsers();
    return true;
  }

  bool Engine::OpenFile(const std::wstring& iPath)
  {
    if (!mCapture) return false;

    std::wstring error;

    if (!mCapture->OpenFile(iPath, error))
    {
      SetLastError(error.empty() ? L"The file could not be opened." : error);
      return false;
    }

    ClearUsers();
    return true;
  }

  void Engine::CloseSource()
  {
    if (mCapture) mCapture->Close();

    std::lock_guard<std::mutex> lock(mFrameMutex);
    mLastFrame.reset();
  }

  void Engine::SetPaused(bool iPaused)
  {
    if (mCapture) mCapture->SetPaused(iPaused);
  }

  void Engine::SetLooping(bool iLooping)
  {
    if (mCapture) mCapture->SetLooping(iLooping);
  }

  void Engine::OnCameraFrame(const cv::Mat& iFrame)
  {
    // The push itself says whether the pipeline took the frame, so a drop is counted here
    // rather than inferred later from what never came back out
    if (mFaceApi.PushCameraFrame(iFrame) != fw::ErrorCode::OK)
    {
      mFramesDropped.fetch_add(1ULL, std::memory_order_acq_rel);
    }

    mFramesCaptured.fetch_add(1ULL, std::memory_order_acq_rel);

    std::lock_guard<std::mutex> lock(mStatsMutex);
    mCaptureRate.Tick(NowMs());
  }

  bool Engine::CreateVideoSwapChain(IDXGISwapChain1** oSwapChain)
  {
    if (!oSwapChain) return false;
    *oSwapChain = nullptr;

    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    if (!mDevice.IsValid()) return false;

    if (!mVideoView)
    {
      mVideoView = std::make_unique<gfx::VideoView>(mDevice);

      if (!mVideoView->Create(1280U, 720U))
      {
        mVideoView.reset();
        SetLastError(L"The video view could not be created.");
        return false;
      }

      mVideoView->SetOptions(mOverlayOptions);
    }

    *oSwapChain = mVideoView->GetSwapChain();
    if (*oSwapChain) (*oSwapChain)->AddRef();

    return *oSwapChain != nullptr;
  }

  bool Engine::ResizeVideoView(int iWidth, int iHeight, double iScaleX, double iScaleY)
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    if (!mVideoView) return false;

    return mVideoView->Resize(static_cast<uint32_t>((std::max)(1, iWidth)),
                              static_cast<uint32_t>((std::max)(1, iHeight)),
                              iScaleX, iScaleY);
  }

  bool Engine::CreateHeadSwapChain(IDXGISwapChain1** oSwapChain)
  {
    if (!oSwapChain) return false;
    *oSwapChain = nullptr;

    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    if (!mDevice.IsValid()) return false;

    if (!mHeadView)
    {
      mHeadView = std::make_unique<gfx::HeadView>(mDevice, mHeadModelPath);

      if (!mHeadView->Create(512U, 512U))
      {
        mHeadView.reset();
        SetLastError(L"The head view could not be created.");
        return false;
      }

      mHeadView->SetOptions(mHeadOptions);
    }

    *oSwapChain = mHeadView->GetSwapChain();
    if (*oSwapChain) (*oSwapChain)->AddRef();

    return *oSwapChain != nullptr;
  }

  bool Engine::ResizeHeadView(int iWidth, int iHeight, double iScaleX, double iScaleY)
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    if (!mHeadView) return false;

    return mHeadView->Resize(static_cast<uint32_t>((std::max)(1, iWidth)),
                             static_cast<uint32_t>((std::max)(1, iHeight)),
                             iScaleX, iScaleY);
  }

  void Engine::SetOverlayOptions(const FeOverlayOptions& iOptions)
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    mOverlayOptions = iOptions;
    if (mVideoView) mVideoView->SetOptions(iOptions);
  }

  void Engine::SetHeadOptions(const FeHeadOptions& iOptions)
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    mHeadOptions = iOptions;
    if (mHeadView) mHeadView->SetOptions(iOptions);
  }

  bool Engine::ViewToFrame(double iViewX, double iViewY, double& oFrameX, double& oFrameY) const
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    return mVideoView && mVideoView->ViewToFrame(iViewX, iViewY, oFrameX, oFrameY);
  }

  ViewFramePtr Engine::PullCompletedFrame()
  {
    cv::Mat image;

    if (mFaceApi.GetResultImage(image) != fw::ErrorCode::OK) return nullptr;
    if (image.empty()) return nullptr;

    auto frame = std::make_shared<ViewFrame>();

    // The pipeline hands out a view onto its own buffer; the copy is what lets the render
    // thread hold on to the frame after the pipeline has moved on
    frame->image = image.clone();
    frame->imageHasOverlay = mPipelineDrawsOverlay.load(std::memory_order_acquire);

    face::FaceResults results;
    const bool hasResults = (mFaceApi.GetResults(results) == fw::ErrorCode::OK);

    frame->frameId = hasResults && !results.empty()
                       ? results.front().frameId
                       : mFaceApi.GetLastFrameId();

    frame->timestampMs = hasResults && !results.empty()
                           ? results.front().timestamp
                           : mFaceApi.GetLastTimestamp();

    frame->latencyMs = frame->timestampMs > 0LL
                         ? static_cast<double>(NowMs() - frame->timestampMs)
                         : 0.0;

    // Straight through: the results are what the renderers and the panels want, in the form
    // the pipeline reports them
    frame->faces = std::move(results);

    return frame;
  }

  void Engine::RenderLoop()
  {
    // The frames come out of another thread and the views are Direct3D objects, so this
    // thread owns COM for as long as it draws
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    auto lastTick = std::chrono::steady_clock::now();

    while (!mRenderStop.load(std::memory_order_acquire))
    {
      const auto now = std::chrono::steady_clock::now();
      const double elapsedSeconds = std::chrono::duration<double>(now - lastTick).count();
      lastTick = now;

      if (ViewFramePtr frame = PullCompletedFrame())
      {
        mFramesProcessed.fetch_add(1ULL, std::memory_order_acq_rel);

        {
          std::lock_guard<std::mutex> lock(mStatsMutex);

          mPipelineRate.Tick(NowMs());
          mLatency.Push(frame->latencyMs);

#ifdef ENABLE_FACE_PROFILER
          const auto measurements = fw::ProfilerDatabase::GetInstance().GetLastMeasurement();

          // The profiler learns about a stage the first time it runs, so the set grows
          // during the first few frames and the index of a name moves with it
          bool layoutChanged = (measurements.size() != mStages.size());

          std::size_t index = 0U;

          for (const auto& measurement : measurements)
          {
            if (!layoutChanged && mStages[index].first != measurement.first) layoutChanged = true;
            ++index;
          }

          if (layoutChanged)
          {
            mStages.clear();

            for (const auto& measurement : measurements)
            {
              mStages.emplace_back(measurement.first, measurement.second.second);
            }

            ++mStageLayoutVersion;
          }
          else
          {
            index = 0U;

            for (const auto& measurement : measurements)
            {
              mStages[index++].second = measurement.second.second;
            }
          }
#endif
        }

        std::lock_guard<std::mutex> lock(mFrameMutex);
        mLastFrame = std::move(frame);
      }

      ViewFramePtr current;
      {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        current = mLastFrame;
      }

      bool deviceAlive = true;

      {
        std::lock_guard<std::mutex> lock(mGraphicsMutex);

        if (mVideoView) deviceAlive = mVideoView->Render(current) && deviceAlive;
        if (mHeadView) deviceAlive = mHeadView->Render(current, elapsedSeconds) && deviceAlive;
      }

      if (!deviceAlive)
      {
        RecoverDevice();
        continue;
      }

      WriteRecordedFrames();

      mFramesRendered.fetch_add(1ULL, std::memory_order_acq_rel);

      {
        std::lock_guard<std::mutex> lock(mStatsMutex);

        const int64_t nowMs = NowMs();

        mRenderRate.Tick(nowMs);

        // Aged even when nothing arrived, so that a stalled source reads as zero rather
        // than as whatever it managed last
        mCaptureRate.Update(nowMs);
        mPipelineRate.Update(nowMs);
      }

      // The presents do not wait for the vertical blank, so the pace is set here - and it
      // is set outside the graphics lock, so that a resize or a new view never has to wait
      // out a frame's worth of sleep to be let in
      constexpr auto sFrameBudget = std::chrono::microseconds(16667);

      const auto spent = std::chrono::steady_clock::now() - now;

      if (spent < sFrameBudget) std::this_thread::sleep_for(sFrameBudget - spent);
    }

    if (SUCCEEDED(comInit)) CoUninitialize();
  }

  void Engine::RecoverDevice()
  {
    std::lock_guard<std::mutex> lock(mGraphicsMutex);

    const bool hadVideo = mVideoView != nullptr;
    const bool hadHead = mHeadView != nullptr;

    mHeadView.reset();
    mVideoView.reset();

    if (!mDevice.Recreate())
    {
      SetLastError(L"The graphics device was lost and could not be rebuilt.");
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      return;
    }

    mRendererGeneration.store(static_cast<int32_t>(mDevice.GetGeneration()), std::memory_order_release);

    // The swap chains of the old device are gone with it. The host sees the generation
    // change in its next snapshot and rebinds its panels to the new ones.
    if (hadVideo)
    {
      mVideoView = std::make_unique<gfx::VideoView>(mDevice);

      if (mVideoView->Create(1280U, 720U)) mVideoView->SetOptions(mOverlayOptions);
      else mVideoView.reset();
    }

    if (hadHead)
    {
      mHeadView = std::make_unique<gfx::HeadView>(mDevice, mHeadModelPath);

      if (mHeadView->Create(512U, 512U)) mHeadView->SetOptions(mHeadOptions);
      else mHeadView.reset();
    }
  }

  void Engine::WriteRecordedFrames()
  {
    if (!mRecording.load(std::memory_order_acquire)) return;

    cv::Mat captured;

    {
      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      if (!mVideoView || !mVideoView->TakeCapture(captured)) return;
    }

    if (captured.empty()) return;

    std::lock_guard<std::mutex> lock(mRecordMutex);

    if (mVideoWriter.IsOpened()) mVideoWriter.Write(captured);
  }

  void Engine::GetSnapshot(FeSnapshot& oSnapshot) const
  {
    oSnapshot = FeSnapshot{};

    oSnapshot.framesCaptured = mFramesCaptured.load(std::memory_order_acquire);
    oSnapshot.framesProcessed = mFramesProcessed.load(std::memory_order_acquire);
    oSnapshot.framesRendered = mFramesRendered.load(std::memory_order_acquire);

    // Both reported rather than inferred: the pipeline says how many frames are waiting,
    // and every drop was counted when the push reported it
    oSnapshot.queueDepth = mFaceApi.GetQueueSize();
    oSnapshot.framesDropped = mFramesDropped.load(std::memory_order_acquire);
    oSnapshot.rendererGeneration = mRendererGeneration.load(std::memory_order_acquire);

    oSnapshot.pipelineDrawsOverlay = mPipelineDrawsOverlay.load(std::memory_order_acquire) ? 1 : 0;

    if (mCapture)
    {
      oSnapshot.isRunning = mCapture->IsOpen() ? 1 : 0;
      oSnapshot.isPaused = mCapture->IsPaused() ? 1 : 0;
      oSnapshot.sourceWidth = mCapture->GetWidth();
      oSnapshot.sourceHeight = mCapture->GetHeight();

      switch (mCapture->GetKind())
      {
        case CaptureSource::Kind::Camera: oSnapshot.sourceKind = FeSourceKind_Camera; break;
        case CaptureSource::Kind::File: oSnapshot.sourceKind = FeSourceKind_File; break;
        default: oSnapshot.sourceKind = FeSourceKind_None; break;
      }
    }

    {
      std::lock_guard<std::mutex> lock(mStatsMutex);

      oSnapshot.captureFps = mCaptureRate.GetRate();
      oSnapshot.pipelineFps = mPipelineRate.GetRate();
      oSnapshot.renderFps = mRenderRate.GetRate();

      oSnapshot.latencyMs = mLatency.GetLast();
      oSnapshot.latencyMinMs = mLatency.GetMinimum();
      oSnapshot.latencyMaxMs = mLatency.GetMaximum();
      oSnapshot.latencyAvgMs = mLatency.GetMean();
      oSnapshot.latencyP95Ms = mLatency.GetPercentile(0.95);

      oSnapshot.stageCount = static_cast<int32_t>((std::min)(mStages.size(), std::size_t{ FE_MAX_STAGES }));
      oSnapshot.stageLayoutVersion = mStageLayoutVersion;

      for (int32_t i = 0; i < oSnapshot.stageCount; ++i)
      {
        oSnapshot.stageMs[i] = mStages[static_cast<std::size_t>(i)].second;
      }
    }

    std::lock_guard<std::mutex> lock(mFrameMutex);

    if (!mLastFrame) return;

    oSnapshot.frameId = static_cast<int32_t>(mLastFrame->frameId);
    oSnapshot.timestampMs = mLastFrame->timestampMs;

    oSnapshot.faceCount = static_cast<int32_t>((std::min)(mLastFrame->faces.size(), std::size_t{ FE_MAX_FACES }));

    for (int32_t i = 0; i < oSnapshot.faceCount; ++i)
    {
      const face::FaceResult& face = mLastFrame->faces[static_cast<std::size_t>(i)];
      FeFace& target = oSnapshot.faces[i];

      target.userId = face.userId;

      // FeTrackState mirrors face::TrackStatus, so there is nothing to translate
      target.state = static_cast<int32_t>(face.status);
      target.hasPose = face.HasPose() ? 1 : 0;
      target.ageSeconds = face.ageSeconds;

      // The pipeline stores roll, pitch and yaw in that order and in radians
      target.rollDeg = fw::rad_to_deg(face.rpy[0]);
      target.pitchDeg = fw::rad_to_deg(face.rpy[1]);
      target.yawDeg = fw::rad_to_deg(face.rpy[2]);

      target.posX = face.position3D[0];
      target.posY = face.position3D[1];
      target.posZ = face.position3D[2];

      target.rectX = face.faceRect.x;
      target.rectY = face.faceRect.y;
      target.rectWidth = face.faceRect.width;
      target.rectHeight = face.faceRect.height;

      target.openMouth = face.expression.openMouth;
      target.openEyeLeft = face.expression.openEyeLeft;
      target.openEyeRight = face.expression.openEyeRight;
      target.browRaise = face.expression.browRaise;
      target.smile = face.expression.smile;
    }
  }

  bool Engine::GetStageName(int iIndex, std::wstring& oName) const
  {
    std::lock_guard<std::mutex> lock(mStatsMutex);

    if (iIndex < 0 || iIndex >= static_cast<int>(mStages.size())) return false;

    oName = ToWide(mStages[static_cast<std::size_t>(iIndex)].first);
    return true;
  }

  bool Engine::ReloadPipeline()
  {
    // The source keeps running: what is torn down here is the graph, and the frames that
    // arrive while it is down are dropped by the queue rather than lost track of
    const bool wasPaused = mCapture && mCapture->IsPaused();

    if (mCapture) mCapture->SetPaused(true);

    mFaceApi.DeInitialize();

    static const cv::FileNode sEmptyNode;

    const bool ok = mFaceApi.Initialize(sEmptyNode) == fw::ErrorCode::OK;

    if (!ok)
    {
      SetLastError(L"The pipeline could not be rebuilt. The settings were saved, but they "
                   L"describe a graph that cannot be created.");
    }
    else
    {
      DetectPipelineOverlay();
    }

    {
      std::lock_guard<std::mutex> lock(mFrameMutex);
      mLastFrame.reset();
    }

    {
      std::lock_guard<std::mutex> lock(mStatsMutex);

      mCaptureRate.Reset();
      mPipelineRate.Reset();
      mLatency.Reset();
      mStages.clear();
      ++mStageLayoutVersion;
    }

    if (mCapture && !wasPaused) mCapture->SetPaused(false);

    return ok;
  }

  void Engine::ClearUsers()
  {
    mFaceApi.Clear();

    std::lock_guard<std::mutex> lock(mFrameMutex);
    mLastFrame.reset();
  }

  void Engine::ForceDetection()
  {
    mFaceApi.SetRunFaceDetector();
  }

  void Engine::SetVerbose(bool iVerbose)
  {
    mFaceApi.SetVerbose(iVerbose);
  }

  bool Engine::SaveFrame(const std::wstring& iPath)
  {
    cv::Mat image;

    {
      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      if (!mVideoView)
      {
        SetLastError(L"There is no view to save yet.");
        return false;
      }

      mVideoView->RequestCapture();
    }

    // The copy is taken by the render thread between drawing and presenting, so this waits
    // for one turn of that loop rather than reading a buffer that is already gone
    for (int attempt = 0; attempt < 60 && image.empty(); ++attempt)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(8));

      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      if (!mVideoView) break;

      mVideoView->TakeCapture(image);
    }

    if (image.empty())
    {
      SetLastError(L"The view could not be captured.");
      return false;
    }

    try
    {
      if (!cv::imwrite(ToUtf8(iPath), image))
      {
        SetLastError(L"The image could not be written to " + iPath + L".");
        return false;
      }
    }
    catch (const cv::Exception&)
    {
      SetLastError(L"The image could not be written to " + iPath + L".");
      return false;
    }

    return true;
  }

  bool Engine::SetRecording(bool iRecording, const std::wstring& iDirectory)
  {
    if (!iRecording)
    {
      mRecording.store(false, std::memory_order_release);

      {
        std::lock_guard<std::mutex> lock(mGraphicsMutex);
        if (mVideoView) mVideoView->SetCaptureEnabled(false);
      }

      std::lock_guard<std::mutex> lock(mRecordMutex);
      mVideoWriter.Close();

      return true;
    }

    const auto& output = mFaceApi.GetConfiguration().GetOutput();

    std::string directory = ToUtf8(iDirectory);
    if (directory.empty()) directory = mFaceApi.GetConfiguration().GetDirectories().output;

    if (!directory.empty() && directory.back() != '\\' && directory.back() != '/') directory += '\\';

    {
      std::lock_guard<std::mutex> lock(mRecordMutex);

      mVideoWriter.Close();
      mVideoWriter.Create(directory, "studio", output.videoFourCC, output.videoFPS);

      if (!mVideoWriter.IsOpened())
      {
        SetLastError(L"The video file could not be opened for writing.");
        return false;
      }
    }

    {
      std::lock_guard<std::mutex> lock(mGraphicsMutex);

      if (!mVideoView)
      {
        SetLastError(L"There is no view to record yet.");
        return false;
      }

      mVideoView->SetCaptureEnabled(true);
    }

    mRecording.store(true, std::memory_order_release);
    return true;
  }
}
