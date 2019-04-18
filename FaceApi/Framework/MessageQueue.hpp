#pragma once

#include "Framework/Util.h"

#include <easyloggingpp/easyloggingpp.h>

#include <chrono>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>
#include <cassert>

namespace fw
{
  template <typename... T>
  struct is_shared_ptr :
    std::false_type
  {
  };

  template <typename... T>
  struct is_shared_ptr<std::shared_ptr<T>... > :
    std::true_type
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

    ~MessageQueue()
    {
      Clear();
    }

    ErrorCode Push(const MessageTuple& iMessageTuple)
    {
      ErrorCode retCode = ErrorCode::OK;
      while ((retCode = TryPush(iMessageTuple)) == ErrorCode::OutOfResources)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode Push(const First& iFirst, const Rest&... iArgs)
    {
      return Push(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode TryPush(const MessageTuple& iMessageTuple)
    {
      int queueSize = GetSize();
      return queueSize >= mBound ? ErrorCode::OutOfResources : InternalPush(iMessageTuple);
    }

    ErrorCode TryPush(const First& iFirst, const Rest&... iArgs)
    {
      return TryPush(std::make_tuple(iFirst, iArgs...));
    }

    ErrorCode Pop(MessageTuple& oDestination)
    {
      ErrorCode retCode = ErrorCode::OK;
      while ((retCode = TryPop(oDestination)) == ErrorCode::NotFound)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode TryPop(MessageTuple& oDestination)
    {
      ErrorCode retCode = TryFront(oDestination);

      if (retCode == ErrorCode::OK)
      {
        InternalPop();
      }

      return retCode;
    }

    ErrorCode Front(MessageTuple& oDestination)
    {
      ErrorCode retCode = ErrorCode::OK;
      while ((retCode = TryFront(oDestination)) == ErrorCode::NotFound)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      return retCode;
    }

    ErrorCode TryFront(MessageTuple& oDestination)
    {
      if (IsEmpty())
      {
        oDestination = MessageTuple();
        return ErrorCode::NotFound;
      }

      return InternalFront(oDestination);
    }

    void Clear()
    {
      std::lock_guard<std::mutex> lock(mMutex);

      while (!mQueue.empty())
      {
        mQueue.pop();
      }

      mTimestampMs = 0LL;
      mSize = 0;
    }

    inline float GetSamplingFPS() const { return mSamplingFPS; }

    inline int GetSize() const { return mSize; }

    inline int GetBound() const { return mBound; }

    inline bool IsEmpty() const { return mSize == 0; }

    inline bool IsFull() const { return mSize >= mBound; }

    void SetBound(int iBound)
    {
      assert(iBound > 0);
      std::lock_guard<std::mutex> lock(mMutex);
      mBound = (std::min)((std::max)(iBound, MIN_BOUND), MAX_BOUND);
    }

    void SetSamplingFPS(float iSamplingFPS)
    {
      assert(iSamplingFPS > 0.0F);
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

    MessageQueue& operator=(const MessageQueue& iOther) = delete;
    MessageQueue(const MessageQueue& iOther) = delete;

    ErrorCode InternalPush(const MessageTuple& iMessageTuple)
    {
      if (mThresholdMs > 0LL)
        InternalFiltering();

      std::lock_guard<std::mutex> lock(mMutex);

      if (mQueue.size() >= mBound) return ErrorCode::OutOfResources;

      const long long currentTimestampMs = fw::get_current_time();

      if (std::llabs(currentTimestampMs - mTimestampMs) <= mSamplingMs)
        return ErrorCode::BadData;

      mTimestampMs = currentTimestampMs;
      mQueue.push(std::make_pair(mTimestampMs, iMessageTuple));
      mSize++;

      assert(mSize == static_cast<int>(mQueue.size()));

      return ErrorCode::OK;
    }

    ErrorCode InternalFront(MessageTuple& oDestination)
    {
      if (mThresholdMs > 0LL)
        InternalFiltering();

      std::lock_guard<std::mutex> lock(mMutex);

      if (mQueue.empty())
      {
        oDestination = MessageTuple();
        return ErrorCode::NotFound;
      }

      oDestination = mQueue.front().second;

      return ErrorCode::OK;
    }

    ErrorCode InternalPop()
    {
      if (mThresholdMs > 0LL)
        InternalFiltering();

      std::lock_guard<std::mutex> lock(mMutex);

      if (mQueue.empty())
      {
        return ErrorCode::NotFound;
      }

      mQueue.pop();
      mSize--;

      assert(mSize == static_cast<int>(mQueue.size()));

      return ErrorCode::OK;
    }

    void InternalFiltering()
    {
      assert(mThresholdMs > 0LL);

      const long long currentTimestampMs = fw::get_current_time();

      // Lock mQueue
      {
        std::lock_guard<std::mutex> lock(mMutex);

        //int startSize = mSize;

        while (!mQueue.empty())
        {
          const long long createTimestampMs = mQueue.front().first;

          if (std::llabs(currentTimestampMs - createTimestampMs) <= mThresholdMs)
            break;

          mQueue.pop();
          mSize--;

          assert(mSize == static_cast<int>(mQueue.size()));
        }

        //if (startSize != mSize)
        //{
        //	LOG(WARNING) << "Number of frames dropped from " << mName << ": " << std::abs(startSize - mSize);
        //}
      }
    }

    inline long long ConvertFpsToMs(float iFPS) const
    {
      assert(iFPS > 0.0F);
      return static_cast<long long>((1.0F / iFPS) * 1000.0F);
    }

    std::mutex mMutex;
    std::queue<std::pair<long long, MessageTuple>> mQueue;

    std::string mName;

    int mSize = 0;
    int mBound = MAX_BOUND;

    float mSamplingFPS = MAX_SAMPLING_RATE_FPS;
    long long mSamplingMs = 1LL;

    long long mTimestampMs = 0LL;
    long long mThresholdMs = -1LL;
  };

  template<typename First, typename... Rest>
  const float fw::MessageQueue<First, Rest...>::MAX_SAMPLING_RATE_FPS = (std::numeric_limits<float>::max)();

  template<typename First, typename... Rest>
  const float fw::MessageQueue<First, Rest...>::MIN_SAMPLING_RATE_FPS = 1.0F;

  template<typename First, typename... Rest>
  const int fw::MessageQueue<First, Rest...>::MAX_BOUND = (std::numeric_limits<int>::max)();

  template<typename First, typename... Rest>
  const int fw::MessageQueue<First, Rest...>::MIN_BOUND = 1;
}
