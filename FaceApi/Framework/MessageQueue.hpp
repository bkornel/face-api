#pragma once

#include "Framework/Util.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/core/base.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>

namespace fw
{
  template <typename... T>
  struct is_shared_ptr : std::false_type
  {
  };

  template <typename... T>
  struct is_shared_ptr<std::shared_ptr<T>...> : std::true_type
  {
  };

  template <typename First, typename... Rest>
  class MessageQueue
  {
    static_assert(is_shared_ptr<First, Rest...>::value, "Template parameter must be std::shared_ptr<T>");

    using MessageTuple = std::tuple<First, Rest...>;

  public:
    explicit MessageQueue(const std::string& iName) :
      MessageQueue(iName, MAX_SAMPLING_RATE_FPS, MAX_BOUND, -1LL)
    {
    }

    MessageQueue(const std::string& iName, float iSamplingFPS, int iBound) :
      MessageQueue(iName, iSamplingFPS, iBound, -1LL)
    {
    }

    MessageQueue(const std::string& iName, float iSamplingFPS, int iBound, long long iThresholdMs) :
      mName(iName)
    {
      SetBound(iBound);
      SetSamplingFPS(iSamplingFPS);
      SetTimestampFiltering(iThresholdMs);
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

        mTimestampMs = 0LL;
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
      mSamplingMs = ConvertFpsToMs(mSamplingFPS);
    }

    void SetTimestampFiltering(long long iThresholdMs)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mThresholdMs = iThresholdMs;
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

      const long long currentTimestampMs = fw::get_current_time();

      if (std::llabs(currentTimestampMs - mTimestampMs) <= mSamplingMs.load())
        return ErrorCode::BadData;

      mTimestampMs = currentTimestampMs;
      mQueue.push(std::make_pair(mTimestampMs, iMessageTuple));
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
      const long long thresholdMs = mThresholdMs.load();
      if (thresholdMs <= 0LL) return;

      const long long currentTimestampMs = fw::get_current_time();
      const int startSize = mSize;

      while (!mQueue.empty())
      {
        const long long createTimestampMs = mQueue.front().first;

        if (std::llabs(currentTimestampMs - createTimestampMs) <= thresholdMs)
          break;

        mQueue.pop();
      }

      mSize = static_cast<int>(mQueue.size());

      if (startSize != mSize)
      {
        mCV.notify_all();
      }
    }

    inline long long ConvertFpsToMs(float iFPS) const
    {
      CV_DbgAssert(iFPS > 0.0F);
      return static_cast<long long>((1.0F / iFPS) * 1000.0F);
    }

    std::mutex mMutex;
    std::condition_variable mCV;
    std::queue<std::pair<long long, MessageTuple>> mQueue;

    std::string mName;

    std::atomic<int> mSize{ 0 };
    std::atomic<int> mBound{ MAX_BOUND };

    std::atomic<float> mSamplingFPS{ MAX_SAMPLING_RATE_FPS };
    std::atomic<long long> mSamplingMs{ 1LL };
    std::atomic<long long> mThresholdMs{ -1LL };

    long long mTimestampMs = 0LL;
  };

  template <typename First, typename... Rest>
  const float fw::MessageQueue<First, Rest...>::MAX_SAMPLING_RATE_FPS = (std::numeric_limits<float>::max)();

  template <typename First, typename... Rest>
  const float fw::MessageQueue<First, Rest...>::MIN_SAMPLING_RATE_FPS = 1.0F;

  template <typename First, typename... Rest>
  const int fw::MessageQueue<First, Rest...>::MAX_BOUND = (std::numeric_limits<int>::max)();

  template <typename First, typename... Rest>
  const int fw::MessageQueue<First, Rest...>::MIN_BOUND = 1;
}
