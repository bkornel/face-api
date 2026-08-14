#include "CaptureSource.h"

#include "TextConversions.h"

#include <chrono>

namespace fe
{
  namespace
  {
    using Clock = std::chrono::steady_clock;
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

      if (interval > Clock::duration::zero())
      {
        next += interval;

        // A decode that fell behind sets the schedule from now, rather than replaying the
        // backlog at full speed the moment it catches up
        if (next < Clock::now()) next = Clock::now();
        else std::this_thread::sleep_until(next);
      }
    }

    mRunning.store(false, std::memory_order_release);
  }
}
