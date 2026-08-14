#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
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
  /// @brief What a push does when the queue is full
  enum class DropPolicy
  {
    /// @brief Turn the new message away. What a recorder wants: nothing is lost, and the
    /// producer is told to slow down.
    Reject = 0,

    /// @brief Make room by throwing the oldest message away. What anything interactive
    /// wants: when the consumer falls behind, the freshest frame is the one worth having,
    /// and holding on to a queue of stale ones only adds the delay of working through them.
    DropOldest = 1
  };

  template <SharedPointer MessageT>
  class MessageQueue
  {
  public:
    struct Statistics
    {
      int size = 0;
      int bound = 0;
      float samplingFPS = 0.0F;

      /// @brief Messages the queue itself threw away: overtaken by a newer one under
      /// DropOldest, or aged past the staleness threshold
      uint64_t dropped = 0ULL;
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

    ErrorCode Push(const MessageT& iMessage)
    {
      std::unique_lock<std::mutex> lock(mMutex);

      for (;;)
      {
        if (mClosed) return ErrorCode::BadState;

        const ErrorCode code = PushLocked(iMessage);
        if (code != ErrorCode::OutOfResources) return code;

        WaitForRoomLocked(lock);
      }
    }

    ErrorCode TryPush(const MessageT& iMessage)
    {
      std::lock_guard<std::mutex> lock(mMutex);

      if (mClosed) return ErrorCode::BadState;

      return PushLocked(iMessage);
    }

    ErrorCode Pop(MessageT& oDestination)
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

    ErrorCode TryPop(MessageT& oDestination)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return PopLocked(oDestination);
    }

    ErrorCode Front(MessageT& oDestination)
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

    ErrorCode TryFront(MessageT& oDestination)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return FrontLocked(oDestination);
    }

    /// @brief Waits until a message is available, the queue is closed, or iTimeout passes.
    /// Lets a consumer sleep on the queue instead of polling it.
    /// @return true when a message is waiting to be popped
    bool WaitForMessage(Milliseconds iTimeout)
    {
      const auto deadline = std::chrono::steady_clock::now() +
                            std::chrono::duration_cast<std::chrono::steady_clock::duration>(iTimeout);

      std::unique_lock<std::mutex> lock(mMutex);

      for (;;)
      {
        FilterLocked();

        if (!mQueue.empty()) return true;
        if (mClosed) return false;

        if (mCV.wait_until(lock, deadline) == std::cv_status::timeout)
        {
          FilterLocked();
          return !mQueue.empty();
        }
      }
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
        mNextAdmission = Timestamp{};
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
      return { static_cast<int>(mQueue.size()), mBound, mSamplingFPS, mDropped };
    }

    void SetDropPolicy(DropPolicy iPolicy)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mPolicy = iPolicy;
    }

    inline DropPolicy GetDropPolicy() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mPolicy;
    }

    /// @brief How many messages the queue has thrown away since it was created
    inline uint64_t GetDroppedCount() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mDropped;
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
        mBound = std::clamp(iBound, MIN_BOUND, MAX_BOUND);
      }

      mCV.notify_all();
    }

    void SetSamplingFPS(float iSamplingFPS)
    {
      assert(iSamplingFPS > 0.0F);
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mSamplingFPS = std::clamp(iSamplingFPS, MIN_SAMPLING_RATE_FPS, MAX_SAMPLING_RATE_FPS);
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
    ErrorCode PushLocked(const MessageT& iMessage)
    {
      FilterLocked();

      if (static_cast<int>(mQueue.size()) >= mBound)
      {
        if (mPolicy == DropPolicy::Reject) return ErrorCode::OutOfResources;

        // The consumer is behind. Everything waiting is older than what has just arrived,
        // so the head of the queue is the least worth keeping.
        mQueue.pop();
        ++mDropped;
      }

      const Timestamp currentTimestamp = now();

      // Admission works in slots rather than "at least a period since the last arrival":
      // measured arrival-to-arrival, a source running exactly at the sampling rate lands
      // every frame a hair inside the period of the one before, and half of them were
      // turned away - a 30 fps camera came through at 15.
      if (currentTimestamp < mNextAdmission) return ErrorCode::BadData;

      // The next slot opens one period after this one, keeping the cadence, so scheduling
      // jitter cannot shave frames off an at-rate source. A source that stalled gets no
      // credit to burst: the cadence restarts from now.
      mNextAdmission = (std::max)(mNextAdmission + std::chrono::duration_cast<WallClock::duration>(mSampling),
                                  currentTimestamp);

      mQueue.emplace(currentTimestamp, iMessage);

      mCV.notify_all();

      return ErrorCode::OK;
    }

    ErrorCode FrontLocked(MessageT& oDestination)
    {
      FilterLocked();

      if (mQueue.empty())
      {
        oDestination = MessageT();
        return ErrorCode::NotFound;
      }

      oDestination = mQueue.front().second;

      return ErrorCode::OK;
    }

    ErrorCode PopLocked(MessageT& oDestination)
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
        mDropped += (startSize - mQueue.size());
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
        mCV.wait_for(ioLock, std::chrono::duration_cast<Microseconds>(*expiry));
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

    std::queue<std::pair<Timestamp, MessageT>> mQueue;

    const std::string mName;

    bool mClosed = false;

    int mBound = MAX_BOUND;
    float mSamplingFPS = MAX_SAMPLING_RATE_FPS;
    Milliseconds mSampling{ 1.0 };
    Milliseconds mThreshold{ -1.0 };

    DropPolicy mPolicy = DropPolicy::Reject;
    uint64_t mDropped = 0ULL;

    /// @brief When the next admission slot opens; epoch admits the first push immediately
    Timestamp mNextAdmission;
  };
}
