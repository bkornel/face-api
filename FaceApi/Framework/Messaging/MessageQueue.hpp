#pragma once


#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include <easyloggingpp/easyloggingpp.h>
#include <memory>
#include <opencv2/core/base.hpp>

#include <atomic>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>

namespace fw
{
  /// @brief Satisfied by std::shared_ptr<T> and nothing else. A type without element_type
  /// fails the requirement rather than the same_as check, so it does not have to be a pointer.
  template <typename T>
  concept SharedPointer = requires { typename T::element_type; } &&
                          std::same_as<T, std::shared_ptr<typename T::element_type>>;

  /// @brief A queue is only ever asked to carry messages, and messages travel as shared_ptr.
  /// The constraint says so in the signature, where a static_assert said it in the body.
  template <SharedPointer First, SharedPointer... Rest>
  class MessageQueue
  {
    using MessageTuple = std::tuple<First, Rest...>;

  public:
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

    MessageQueue& operator=(const MessageQueue& iOther) = delete;

    ~MessageQueue()
    {
      Clear();
    }

    MessageQueue(const MessageQueue& iOther) = delete;

    ErrorCode Push(const MessageTuple& iMessageTuple)
    {
      ErrorCode retCode = ErrorCode::OK;
      std::unique_lock<std::mutex> lock(mMutex);

      while ((retCode = PushLocked(iMessageTuple)) == ErrorCode::OutOfResources)
      {
        mCV.wait_for(lock, std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode Push(const First& iFirst, const Rest&... iArgs)
    {
      return Push(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode TryPush(const MessageTuple& iMessageTuple)
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return PushLocked(iMessageTuple);
    }

    ErrorCode TryPush(const First& iFirst, const Rest&... iArgs)
    {
      return TryPush(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode Pop(MessageTuple& oDestination)
    {
      ErrorCode retCode = ErrorCode::OK;
      std::unique_lock<std::mutex> lock(mMutex);

      while ((retCode = PopLocked(oDestination)) == ErrorCode::NotFound)
      {
        mCV.wait_for(lock, std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode TryPop(MessageTuple& oDestination)
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return PopLocked(oDestination);
    }

    ErrorCode Front(MessageTuple& oDestination)
    {
      ErrorCode retCode = ErrorCode::OK;
      std::unique_lock<std::mutex> lock(mMutex);

      while ((retCode = FrontLocked(oDestination)) == ErrorCode::NotFound)
      {
        mCV.wait_for(lock, std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode TryFront(MessageTuple& oDestination)
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return FrontLocked(oDestination);
    }

    void Clear()
    {
      {
        std::lock_guard<std::mutex> lock(mMutex);

        while (!mQueue.empty())
        {
          mQueue.pop();
        }

        mTimestamp = Timestamp{};
        mSize = 0;
      }

      mCV.notify_all();
    }

    inline float GetSamplingFPS() const
    {
      return mSamplingFPS;
    }

    inline int GetSize() const
    {
      return mSize;
    }

    inline int GetBound() const
    {
      return mBound;
    }

    inline bool IsEmpty() const
    {
      return mSize == 0;
    }

    inline bool IsFull() const
    {
      return mSize >= mBound;
    }

    void SetBound(int iBound)
    {
      CV_DbgAssert(iBound > 0);
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mBound = (std::min)((std::max)(iBound, MIN_BOUND), MAX_BOUND);
      }

      mCV.notify_all();
    }

    void SetSamplingFPS(float iSamplingFPS)
    {
      CV_DbgAssert(iSamplingFPS > 0.0F);
      std::lock_guard<std::mutex> lock(mMutex);
      mSamplingFPS = (std::min)((std::max)(iSamplingFPS, MIN_SAMPLING_RATE_FPS), MAX_SAMPLING_RATE_FPS);
      mSampling = ConvertFpsToDuration(mSamplingFPS);
    }

    void SetTimestampFiltering(Milliseconds iThreshold)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mThreshold = iThreshold;
    }

  private:
    const static float MAX_SAMPLING_RATE_FPS;
    const static float MIN_SAMPLING_RATE_FPS;

    const static int MAX_BOUND;
    const static int MIN_BOUND;

    ErrorCode PushLocked(const MessageTuple& iMessageTuple)
    {
      FilterLocked();

      if (static_cast<int>(mQueue.size()) >= mBound.load()) return ErrorCode::OutOfResources;

      const Timestamp currentTimestamp = now();

      if (elapsed(mTimestamp, currentTimestamp) <= mSampling.load())
        return ErrorCode::BadData;

      mTimestamp = currentTimestamp;
      mQueue.push(std::make_pair(mTimestamp, iMessageTuple));
      mSize = static_cast<int>(mQueue.size());

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
      const ErrorCode retCode = FrontLocked(oDestination);

      if (retCode == ErrorCode::OK)
      {
        mQueue.pop();
        mSize = static_cast<int>(mQueue.size());

        mCV.notify_all();
      }

      return retCode;
    }

    void FilterLocked()
    {
      const Milliseconds threshold = mThreshold.load();
      if (threshold.count() <= 0.0) return;

      const Timestamp currentTimestamp = now();
      const int startSize = mSize;

      while (!mQueue.empty())
      {
        const Timestamp createTimestamp = mQueue.front().first;

        if (elapsed(createTimestamp, currentTimestamp) <= threshold)
          break;

        mQueue.pop();
      }

      mSize = static_cast<int>(mQueue.size());

      if (startSize != mSize)
      {
        mCV.notify_all();
      }
    }

    inline Milliseconds ConvertFpsToDuration(float iFPS) const
    {
      CV_DbgAssert(iFPS > 0.0F);
      return Milliseconds((1.0 / iFPS) * 1000.0);
    }

    std::mutex mMutex;
    std::condition_variable mCV;
    std::queue<std::pair<Timestamp, MessageTuple>> mQueue;

    std::string mName;

    std::atomic<int> mSize{ 0 };
    std::atomic<int> mBound{ MAX_BOUND };

    std::atomic<float> mSamplingFPS{ MAX_SAMPLING_RATE_FPS };
    std::atomic<Milliseconds> mSampling{ Milliseconds(1.0) };
    std::atomic<Milliseconds> mThreshold{ Milliseconds(-1.0) };

    Timestamp mTimestamp;
  };

  template <SharedPointer First, SharedPointer... Rest>
  const float fw::MessageQueue<First, Rest...>::MAX_SAMPLING_RATE_FPS = (std::numeric_limits<float>::max)();

  template <SharedPointer First, SharedPointer... Rest>
  const float fw::MessageQueue<First, Rest...>::MIN_SAMPLING_RATE_FPS = 1.0F;

  template <SharedPointer First, SharedPointer... Rest>
  const int fw::MessageQueue<First, Rest...>::MAX_BOUND = (std::numeric_limits<int>::max)();

  template <SharedPointer First, SharedPointer... Rest>
  const int fw::MessageQueue<First, Rest...>::MIN_BOUND = 1;
}
