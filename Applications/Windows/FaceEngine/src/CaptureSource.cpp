#include "CaptureSource.h"

#include <windows.h>

#include <chrono>

namespace fe
{
  namespace
  {
    using Clock = std::chrono::steady_clock;

    std::string ToUtf8(const std::wstring& iText)
    {
      if (iText.empty()) return {};

      const int size = WideCharToMultiByte(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()),
                                           nullptr, 0, nullptr, nullptr);
      if (size <= 0) return {};

      std::string result(static_cast<std::size_t>(size), '\0');
      WideCharToMultiByte(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()),
                          result.data(), size, nullptr, nullptr);
      return result;
    }
  }

  CaptureSource::CaptureSource(FrameSink iSink) :
    mSink(std::move(iSink))
  {
  }

  CaptureSource::~CaptureSource()
  {
    Close();
  }

  bool CaptureSource::OpenCamera(int iIndex, int iWidth, int iHeight, double iFps, std::wstring& oError)
  {
    Close();

    std::lock_guard<std::mutex> lock(mCaptureMutex);

    // DirectShow rather than the default backend: it honours a resolution set before the
    // first grab, which Media Foundation only does for the modes it happens to expose.
    if (!mCapture.open(iIndex, cv::CAP_DSHOW) && !mCapture.open(iIndex, cv::CAP_ANY))
    {
      oError = L"No camera could be opened at index " + std::to_wstring(iIndex) + L".";
      return false;
    }

    if (iWidth > 0 && iHeight > 0)
    {
      mCapture.set(cv::CAP_PROP_FRAME_WIDTH, iWidth);
      mCapture.set(cv::CAP_PROP_FRAME_HEIGHT, iHeight);
    }

    if (iFps > 0.0) mCapture.set(cv::CAP_PROP_FPS, iFps);

    return Start(Kind::Camera, 0.0);
  }

  bool CaptureSource::OpenFile(const std::wstring& iPath, std::wstring& oError)
  {
    Close();

    std::lock_guard<std::mutex> lock(mCaptureMutex);

    if (!mCapture.open(ToUtf8(iPath), cv::CAP_ANY))
    {
      oError = L"The file could not be opened: " + iPath;
      return false;
    }

    // A file is decoded as fast as the disk allows, so it is the nominal rate that decides
    // how often a frame is handed on. A camera paces itself.
    double fps = mCapture.get(cv::CAP_PROP_FPS);
    if (!(fps > 1.0) || fps > 240.0) fps = 30.0;

    return Start(Kind::File, fps);
  }

  bool CaptureSource::Start(Kind iKind, double iNominalFps)
  {
    if (!mCapture.isOpened()) return false;

    mWidth.store(static_cast<int>(mCapture.get(cv::CAP_PROP_FRAME_WIDTH)), std::memory_order_release);
    mHeight.store(static_cast<int>(mCapture.get(cv::CAP_PROP_FRAME_HEIGHT)), std::memory_order_release);
    mKind.store(iKind, std::memory_order_release);
    mFrameCount.store(0ULL, std::memory_order_release);

    mStopSignal.store(false, std::memory_order_release);
    mPaused.store(false, std::memory_order_release);
    mRunning.store(true, std::memory_order_release);

    mThread = std::thread(&CaptureSource::Run, this, iNominalFps);

    return true;
  }

  void CaptureSource::Close()
  {
    StopThread();

    std::lock_guard<std::mutex> lock(mCaptureMutex);

    if (mCapture.isOpened()) mCapture.release();

    mKind.store(Kind::None, std::memory_order_release);
    mWidth.store(0, std::memory_order_release);
    mHeight.store(0, std::memory_order_release);

    std::lock_guard<std::mutex> rateLock(mRateMutex);
    mMeasuredFps = 0.0;
  }

  void CaptureSource::StopThread()
  {
    mStopSignal.store(true, std::memory_order_release);

    if (mThread.joinable()) mThread.join();

    mRunning.store(false, std::memory_order_release);
  }

  void CaptureSource::Run(double iNominalFps)
  {
    const auto interval = iNominalFps > 0.0
                            ? std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / iNominalFps))
                            : Clock::duration::zero();

    auto next = Clock::now();
    auto windowStart = Clock::now();
    int windowFrames = 0;

    cv::Mat frame;

    while (!mStopSignal.load(std::memory_order_acquire))
    {
      if (mPaused.load(std::memory_order_acquire))
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));

        // Otherwise the pacing would try to catch up with every frame the pause skipped
        next = Clock::now();
        continue;
      }

      bool grabbed = false;
      {
        std::lock_guard<std::mutex> lock(mCaptureMutex);

        if (!mCapture.isOpened()) break;

        grabbed = mCapture.read(frame) && !frame.empty();

        if (!grabbed && mKind.load(std::memory_order_acquire) == Kind::File &&
            mLooping.load(std::memory_order_acquire))
        {
          mCapture.set(cv::CAP_PROP_POS_FRAMES, 0.0);
          grabbed = mCapture.read(frame) && !frame.empty();
        }
      }

      // End of a file that is not looping, or a camera that was unplugged
      if (!grabbed) break;

      mFrameCount.fetch_add(1ULL, std::memory_order_acq_rel);

      if (mSink) mSink(frame);

      ++windowFrames;
      const auto now = Clock::now();
      const auto windowMs = std::chrono::duration<double, std::milli>(now - windowStart).count();

      if (windowMs >= 500.0)
      {
        std::lock_guard<std::mutex> lock(mRateMutex);
        mMeasuredFps = windowFrames * 1000.0 / windowMs;
        windowFrames = 0;
        windowStart = now;
      }

      if (interval > Clock::duration::zero())
      {
        next += interval;

        // A decode that fell behind sets the schedule from now, rather than replaying the
        // backlog at full speed the moment it catches up
        if (next < now) next = now;
        else std::this_thread::sleep_until(next);
      }
    }

    mRunning.store(false, std::memory_order_release);
  }

  double CaptureSource::GetMeasuredFps() const
  {
    std::lock_guard<std::mutex> lock(mRateMutex);
    return mMeasuredFps;
  }
}
