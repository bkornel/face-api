#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <tuple>
#include <utility>

namespace fw
{
  /// @brief Satisfied by std::shared_ptr<T> and nothing else. A type without element_type
  /// fails the requirement rather than the same_as check, so it does not have to be a pointer.
  template <typename T>
  concept SharedPointer = requires { typename T::element_type; } &&
                          std::same_as<T, std::shared_ptr<typename T::element_type>>;

  /// @brief A bounded, sampled queue of messages. A push is turned away with OutOfResources
  /// when the queue is full, with BadData when it came sooner than the sampling rate allows,
  /// and with BadState once the queue is closed. Only fullness is worth waiting for, so only
  /// Push() waits. Close() releases every waiting thread and is not undone by Clear(); the
  /// owner still has to keep the queue alive until those threads have left it.
  ///
  /// This class is thread-safe.
  template <SharedPointer First, SharedPointer... Rest>
  class MessageQueue
  {
    using MessageTuple = std::tuple<First, Rest...>;

  public:
    struct Statistics
    {
      int size = 0;
      int bound = 0;
      float samplingFPS = 0.0F;
    };

    static constexpr float MAX_SAMPLING_RATE_FPS = (std::numeric_limits<float>::max)();
    static constexpr float MIN_SAMPLING_RATE_FPS = 1.0F;

    static constexpr int MAX_BOUND = (std::numeric_limits<int>::max)();
    static constexpr int MIN_BOUND = 1;

    explicit MessageQueue(const std::string& iName) :
      MessageQueue(iName, MAX_SAMPLING_RATE_FPS, MAX_BOUND, Milliseconds(-1.0))
    {
    }

    MessageQueue(const std::string& iName, float iSamplingFPS, int iBound) :
      MessageQueue(iName, iSamplingFPS, iBound, Milliseconds(-1.0))
    {
    }

    MessageQueue(const std::string& iName, float iSamplingFPS, int iBound, Milliseconds iThreshold) :
      mName(iName)
    {
      SetBound(iBound);
      SetSamplingFPS(iSamplingFPS);
      SetTimestampFiltering(iThreshold);
    }

    MessageQueue(const MessageQueue& iOther) = delete;

    MessageQueue& operator=(const MessageQueue& iOther) = delete;

    ~MessageQueue()
    {
      Close();
      Clear();
    }

    ErrorCode Push(const MessageTuple& iMessageTuple)
    {
      std::unique_lock<std::mutex> lock(mMutex);

      for (;;)
      {
        if (mClosed) return ErrorCode::BadState;

        const ErrorCode code = PushLocked(iMessageTuple);
        if (code != ErrorCode::OutOfResources) return code;

        WaitForRoomLocked(lock);
      }
    }

    ErrorCode Push(const First& iFirst, const Rest&... iArgs)
    {
      return Push(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode TryPush(const MessageTuple& iMessageTuple)
    {
      std::lock_guard<std::mutex> lock(mMutex);

      if (mClosed) return ErrorCode::BadState;

      return PushLocked(iMessageTuple);
    }

    ErrorCode TryPush(const First& iFirst, const Rest&... iArgs)
    {
      return TryPush(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode Pop(MessageTuple& oDestination)
    {
      std::unique_lock<std::mutex> lock(mMutex);

      for (;;)
      {
        const ErrorCode code = PopLocked(oDestination);
        if (code != ErrorCode::NotFound) return code;

        // A closed queue can still be drained, but once empty nothing more is coming
        if (mClosed) return ErrorCode::BadState;

        mCV.wait(lock);
      }
    }

    ErrorCode TryPop(MessageTuple& oDestination)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return PopLocked(oDestination);
    }

    ErrorCode Front(MessageTuple& oDestination)
    {
      std::unique_lock<std::mutex> lock(mMutex);

      for (;;)
      {
        const ErrorCode code = FrontLocked(oDestination);
        if (code != ErrorCode::NotFound) return code;

        if (mClosed) return ErrorCode::BadState;

        mCV.wait(lock);
      }
    }

    ErrorCode TryFront(MessageTuple& oDestination)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return FrontLocked(oDestination);
    }

    void Close()
    {
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mClosed = true;
      }

      mCV.notify_all();
    }

    inline bool IsClosed() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mClosed;
    }

    void Clear()
    {
      {
        std::lock_guard<std::mutex> lock(mMutex);

        mQueue = {};
        mTimestamp = Timestamp{};
      }

      mCV.notify_all();
    }

    inline const std::string& GetName() const
    {
      return mName;
    }

    inline Statistics GetStatistics() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return { static_cast<int>(mQueue.size()), mBound, mSamplingFPS };
    }

    inline float GetSamplingFPS() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mSamplingFPS;
    }

    inline int GetSize() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return static_cast<int>(mQueue.size());
    }

    inline int GetBound() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mBound;
    }

    inline bool IsEmpty() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mQueue.empty();
    }

    inline bool IsFull() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return static_cast<int>(mQueue.size()) >= mBound;
    }

    void SetBound(int iBound)
    {
      assert(iBound > 0);
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mBound = (std::min)((std::max)(iBound, MIN_BOUND), MAX_BOUND);
      }

      mCV.notify_all();
    }

    void SetSamplingFPS(float iSamplingFPS)
    {
      assert(iSamplingFPS > 0.0F);
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mSamplingFPS = (std::min)((std::max)(iSamplingFPS, MIN_SAMPLING_RATE_FPS), MAX_SAMPLING_RATE_FPS);
        mSampling = ConvertFpsToDuration(mSamplingFPS);
      }

      mCV.notify_all();
    }

    void SetTimestampFiltering(Milliseconds iThreshold)
    {
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mThreshold = iThreshold;
      }

      mCV.notify_all();
    }

  private:
    ErrorCode PushLocked(const MessageTuple& iMessageTuple)
    {
      FilterLocked();

      if (static_cast<int>(mQueue.size()) >= mBound) return ErrorCode::OutOfResources;

      const Timestamp currentTimestamp = now();

      if (elapsed(mTimestamp, currentTimestamp) <= mSampling)
        return ErrorCode::BadData;

      mTimestamp = currentTimestamp;
      mQueue.emplace(mTimestamp, iMessageTuple);

      mCV.notify_all();

      return ErrorCode::OK;
    }

    ErrorCode FrontLocked(MessageTuple& oDestination)
    {
      FilterLocked();

      if (mQueue.empty())
      {
        oDestination = MessageTuple();
        return ErrorCode::NotFound;
      }

      oDestination = mQueue.front().second;

      return ErrorCode::OK;
    }

    ErrorCode PopLocked(MessageTuple& oDestination)
    {
      const ErrorCode code = FrontLocked(oDestination);

      if (code == ErrorCode::OK)
      {
        mQueue.pop();

        mCV.notify_all();
      }

      return code;
    }

    void FilterLocked()
    {
      if (mThreshold.count() <= 0.0) return;

      const Timestamp currentTimestamp = now();
      const std::size_t startSize = mQueue.size();

      while (!mQueue.empty() && elapsed(mQueue.front().first, currentTimestamp) > mThreshold)
        mQueue.pop();

      if (startSize != mQueue.size())
      {
        mCV.notify_all();
      }
    }

    std::optional<Milliseconds> TimeToNextExpiryLocked() const
    {
      if (mThreshold.count() <= 0.0 || mQueue.empty()) return std::nullopt;

      const Milliseconds age = elapsed(mQueue.front().first, now());

      return (age >= mThreshold) ? Milliseconds(0.0) : Milliseconds(mThreshold - age);
    }

    // Room appears either because someone popped, which notifies, or because the oldest
    // message aged past the threshold, which nothing announces - hence the bounded wait.
    void WaitForRoomLocked(std::unique_lock<std::mutex>& ioLock)
    {
      const std::optional<Milliseconds> expiry = TimeToNextExpiryLocked();

      if (expiry)
        mCV.wait_for(ioLock, std::chrono::duration_cast<std::chrono::microseconds>(*expiry));
      else
        mCV.wait(ioLock);
    }

    static Milliseconds ConvertFpsToDuration(float iFPS)
    {
      assert(iFPS > 0.0F);
      return Milliseconds((1.0 / iFPS) * 1000.0);
    }

    mutable std::mutex mMutex;
    std::condition_variable mCV;

    std::queue<std::pair<Timestamp, MessageTuple>> mQueue;

    const std::string mName;

    bool mClosed = false;

    int mBound = MAX_BOUND;
    float mSamplingFPS = MAX_SAMPLING_RATE_FPS;
    Milliseconds mSampling{ 1.0 };
    Milliseconds mThreshold{ -1.0 };

    Timestamp mTimestamp;
  };
}
