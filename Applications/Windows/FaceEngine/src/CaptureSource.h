#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>

namespace fe
{
  /// @brief A camera or a video file, read on its own thread and handed out one frame at a
  /// time.
  ///
  /// The thread is what keeps a slow source - a file being decoded, a camera waking up -
  /// from blocking either the UI or the renderer. The sink runs on this thread, so it must
  /// do no more than push the frame into the pipeline.
  class CaptureSource
  {
  public:
    enum class Kind
    {
      None = 0,
      Camera = 1,
      File = 2
    };

    using FrameSink = std::function<void(const cv::Mat&)>;

    explicit CaptureSource(FrameSink iSink);

    CaptureSource(const CaptureSource& iOther) = delete;

    ~CaptureSource();

    CaptureSource& operator=(const CaptureSource& iOther) = delete;

    /// @param iWidth, iHeight Requested capture size, 0 to leave the driver's default
    bool OpenCamera(int iIndex, int iWidth, int iHeight, double iFps, std::wstring& oError);

    bool OpenFile(const std::wstring& iPath, std::wstring& oError);

    void Close();

    inline bool IsOpen() const
    {
      return mRunning.load(std::memory_order_acquire);
    }

    inline Kind GetKind() const
    {
      return mKind.load(std::memory_order_acquire);
    }

    inline void SetPaused(bool iPaused)
    {
      mPaused.store(iPaused, std::memory_order_release);
    }

    inline bool IsPaused() const
    {
      return mPaused.load(std::memory_order_acquire);
    }

    inline void SetLooping(bool iLooping)
    {
      mLooping.store(iLooping, std::memory_order_release);
    }

    inline int GetWidth() const
    {
      return mWidth.load(std::memory_order_acquire);
    }

    inline int GetHeight() const
    {
      return mHeight.load(std::memory_order_acquire);
    }

    inline uint64_t GetFrameCount() const
    {
      return mFrameCount.load(std::memory_order_acquire);
    }

    /// @brief Frames per second measured over the last second of grabs
    double GetMeasuredFps() const;

  private:
    /// @brief Takes over a capture that is already open and starts the reader thread
    bool Start(Kind iKind, double iNominalFps);

    void StopThread();

    void Run(double iNominalFps);

    FrameSink mSink;

    std::thread mThread;
    std::atomic<bool> mRunning{ false };
    std::atomic<bool> mStopSignal{ false };
    std::atomic<bool> mPaused{ false };
    std::atomic<bool> mLooping{ true };

    std::atomic<Kind> mKind{ Kind::None };
    std::atomic<int> mWidth{ 0 };
    std::atomic<int> mHeight{ 0 };
    std::atomic<uint64_t> mFrameCount{ 0ULL };

    /// @brief Guards the capture against Close() running while the thread is grabbing
    std::mutex mCaptureMutex;
    cv::VideoCapture mCapture;

    mutable std::mutex mRateMutex;
    double mMeasuredFps = 0.0;
  };
}
