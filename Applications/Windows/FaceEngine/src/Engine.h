#pragma once

#include "FaceEngine.h"

#include "CaptureSource.h"
#include "gfx/GraphicsDevice.h"
#include "gfx/HeadView.h"
#include "gfx/VideoView.h"

#include "Framework/Metrics.h"
#include "ViewFrame.h"

#include "FaceResult.h"
#include "Framework/Imaging/VideoWriter.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fe
{
  /// @brief What the flat interface in FaceEngine.h is a facade over.
  ///
  /// Three threads meet here and each one has a job it does not share. The capture thread
  /// reads a source and hands frames to the pipeline. The pipeline runs on its own thread
  /// inside FaceApi. The render thread takes finished frames and draws them. The calling
  /// thread - the application's UI - only ever changes settings and copies out a snapshot,
  /// so a slow paint can never hold up a frame and a slow frame can never hold up the UI.
  class Engine
  {
  public:
    Engine();

    Engine(const Engine& iOther) = delete;

    ~Engine();

    Engine& operator=(const Engine& iOther) = delete;

    /// @param iWorkingDirectory Where settings.json and the model files are
    bool Initialize(const std::wstring& iWorkingDirectory);

    void Shutdown();

    inline bool IsInitialized() const
    {
      return mInitialized;
    }

    /// @name Source
    /// @{

    int EnumerateCameras(int iMaxProbe);

    bool GetCameraName(int iIndex, std::wstring& oName) const;

    bool OpenCamera(int iIndex, int iWidth, int iHeight, double iFps);

    bool OpenFile(const std::wstring& iPath);

    void CloseSource();

    void SetPaused(bool iPaused);

    void SetLooping(bool iLooping);

    /// @}
    /// @name Views
    /// @{

    bool CreateVideoSwapChain(IDXGISwapChain1** oSwapChain);

    bool ResizeVideoView(int iWidth, int iHeight, double iScaleX, double iScaleY);

    bool CreateHeadSwapChain(IDXGISwapChain1** oSwapChain);

    bool ResizeHeadView(int iWidth, int iHeight, double iScaleX, double iScaleY);

    void SetOverlayOptions(const FeOverlayOptions& iOptions);

    void SetHeadOptions(const FeHeadOptions& iOptions);

    bool ViewToFrame(double iViewX, double iViewY, double& oFrameX, double& oFrameY) const;

    /// @}
    /// @name Results and control
    /// @{

    void GetSnapshot(FeSnapshot& oSnapshot) const;

    bool GetStageName(int iIndex, std::wstring& oName) const;

    bool ReloadPipeline();

    void ClearUsers();

    void ForceDetection();

    void SetVerbose(bool iVerbose);

    bool SaveFrame(const std::wstring& iPath);

    bool SetRecording(bool iRecording, const std::wstring& iDirectory);

    /// @}

    inline std::wstring GetLastError() const
    {
      std::lock_guard<std::mutex> lock(mErrorMutex);
      return mLastError;
    }

  private:
    void SetLastError(const std::wstring& iMessage);

    /// @brief Runs on the capture thread: hands the frame to the pipeline and counts it
    void OnCameraFrame(const cv::Mat& iFrame);

    /// @brief The render thread
    void RenderLoop();

    /// @brief Pulls one completed frame out of the pipeline, or returns null
    ViewFramePtr PullCompletedFrame();

    /// @brief Reads settings.json to find out whether the pipeline's own Visualizer is
    /// wired into the last module, in which case the frames already carry an overlay
    void DetectPipelineOverlay();

    /// @brief Rebuilds the device and both views after a device loss
    void RecoverDevice();

    void WriteRecordedFrames();

    bool mInitialized = false;

    std::string mWorkingDirectory;

    /// @brief Serialises everything that touches the graphics device: the render thread
    /// draws under it, and the UI thread creates and resizes views under it
    mutable std::mutex mGraphicsMutex;
    gfx::GraphicsDevice mDevice;
    std::unique_ptr<gfx::VideoView> mVideoView;
    std::unique_ptr<gfx::HeadView> mHeadView;

    std::unique_ptr<CaptureSource> mCapture;

    std::thread mRenderThread;
    std::atomic<bool> mRenderStop{ false };

    /// @brief The last completed frame, written by the render thread; the mutex is for
    /// GetSnapshot, which comes from the UI thread
    mutable std::mutex mFrameMutex;
    ViewFramePtr mLastFrame;

    mutable std::mutex mStatsMutex;
    fw::RateCounter mCaptureRate;
    fw::RateCounter mPipelineRate;
    fw::RateCounter mRenderRate;
    fw::SampleStatistics mLatency;
    std::vector<std::pair<std::string, double>> mStages;

    /// @brief Changes whenever the names in mStages change, so that a host can tell that
    /// the index it read a name against no longer means the same stage
    int32_t mStageLayoutVersion = 0;

    std::atomic<uint64_t> mFramesCaptured{ 0ULL };
    std::atomic<uint64_t> mFramesProcessed{ 0ULL };
    std::atomic<uint64_t> mFramesRendered{ 0ULL };

    /// @brief How many frames the pipeline's image queue holds, read from settings.json.
    /// A push that meets a full queue is one the pipeline will drop.
    std::atomic<uint64_t> mQueueBound{ 10ULL };

    std::atomic<bool> mPipelineDrawsOverlay{ false };

    /// @brief Mirrors the graphics device's generation without having to take its mutex,
    /// which the render thread holds for most of every frame
    std::atomic<int32_t> mRendererGeneration{ 1 };

    std::mutex mRecordMutex;
    face::VideoWriter mVideoWriter;
    std::atomic<bool> mRecording{ false };

    mutable std::mutex mErrorMutex;
    std::wstring mLastError;

    std::vector<std::wstring> mCameraNames;

    FeOverlayOptions mOverlayOptions{};
    FeHeadOptions mHeadOptions{};
  };
}
