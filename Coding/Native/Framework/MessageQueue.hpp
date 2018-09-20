#pragma once

#include <chrono>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>

#include <easyloggingpp/easyloggingpp.h>

#include "Util.h"

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
		explicit MessageQueue(const std::string& name) :
			MessageQueue(name, MAX_SAMPLING_RATE_FPS, MAX_BOUND, -1)
		{
		}

		MessageQueue(const std::string& name, float samplingFPS, int bound) :
			MessageQueue(name, samplingFPS, bound, -1)
		{
		}

		MessageQueue(const std::string& name, float samplingFPS, int bound, long long thresholdMs) :
			mName(name)
		{
			SetBound(bound);
			SetSamplingFPS(samplingFPS);
			SetTimestampFiltering(thresholdMs);
		}

		~MessageQueue()
		{
			Clear();
		}

		ErrorCode Push(const MessageTuple& messageTuple)
		{
			ErrorCode retCode = ErrorCode::OK;
			while ((retCode = TryPush(messageTuple)) == ErrorCode::OutOfResources)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			return retCode;
		}

		ErrorCode Push(const First& first, const Rest&... args)
		{
			return Push(std::make_tuple(first, args...));
		}

		ErrorCode TryPush(const MessageTuple& messageTuple)
		{
			int queueSize = GetSize();
			return queueSize >= mBound ? ErrorCode::OutOfResources : InternalPush(messageTuple);
		}

		ErrorCode TryPush(const First& first, const Rest&... args)
		{
			return TryPush(std::make_tuple(first, args...));
		}

		ErrorCode Pop(MessageTuple& outDestination)
		{
			ErrorCode retCode = ErrorCode::OK;
			while ((retCode = TryPop(outDestination)) == ErrorCode::NotFound)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			return retCode;
		}

		ErrorCode TryPop(MessageTuple& outDestination)
		{
			ErrorCode retCode = TryFront(outDestination);

			if (retCode == ErrorCode::OK)
			{
				InternalPop();
			}

			return retCode;
		}

		ErrorCode Front(MessageTuple& outDestination)
		{
			ErrorCode retCode = ErrorCode::OK;
			while ((retCode = TryFront(outDestination)) == ErrorCode::NotFound)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			return retCode;
		}

		ErrorCode TryFront(MessageTuple& outDestination)
		{
			if (IsEmpty())
			{
				outDestination = MessageTuple();
				return ErrorCode::NotFound;
			}

			return InternalFront(outDestination);
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lock(mMutex);

			while (!mQueue.empty())
			{
				mQueue.pop();
			}

			mTimestampMs = 0;
			mSize = 0;
		}

		inline float GetSamplingFPS() const { return mSamplingFPS; }

		inline int GetSize() const { return mSize; }

		inline int GetBound() const { return mBound; }

		inline bool IsEmpty() const { return mSize == 0; }

		inline bool IsFull() const { return mSize >= mBound; }

		void SetBound(int bound)
		{
			assert(bound > 0);
			std::lock_guard<std::mutex> lock(mMutex);
			mBound = (std::min)((std::max)(bound, MIN_BOUND), MAX_BOUND);
		}

		void SetSamplingFPS(float samplingFPS)
		{
			assert(samplingFPS > 0.0F);
			std::lock_guard<std::mutex> lock(mMutex);
			mSamplingFPS = (std::min)((std::max)(samplingFPS, MIN_SAMPLING_RATE_FPS), MAX_SAMPLING_RATE_FPS);
			mSamplingMs = ConvertFpsToMs(mSamplingFPS);
		}

		void SetTimestampFiltering(long long thresholdMs)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			mThresholdMs = thresholdMs;
		}

	private:
		const static float MAX_SAMPLING_RATE_FPS;
		const static float MIN_SAMPLING_RATE_FPS;

		const static int MAX_BOUND;
		const static int MIN_BOUND;

		MessageQueue& operator=(const MessageQueue& rhs) = delete;
		MessageQueue(const MessageQueue& rhs) = delete;

		ErrorCode InternalPush(const MessageTuple& messageTuple)
		{
			if (mThresholdMs > 0)
				InternalFiltering();

			std::lock_guard<std::mutex> lock(mMutex);

			if (mQueue.size() >= mBound) return ErrorCode::OutOfResources;

			// Mintavételezési frekvencia
			const long long currentTimestampMs = fw::get_current_time();

#if WIN32
			if (std::llabs(currentTimestampMs - mTimestampMs) <= mSamplingMs)
				return ErrorCode::BadData;
#else
			if (abs(currentTimestampMs - mTimestampMs) <= mSamplingMs)
				return ErrorCode::BadData;
#endif

			// Új üzenet beszúrása
			mTimestampMs = currentTimestampMs;

			mQueue.push(std::make_pair(mTimestampMs, messageTuple));
			mSize++;

			assert(mSize == static_cast<int>(mQueue.size()));

			return ErrorCode::OK;
		}

		ErrorCode InternalFront(MessageTuple& outDestination)
		{
			if (mThresholdMs > 0)
				InternalFiltering();

			std::lock_guard<std::mutex> lock(mMutex);

			if (mQueue.empty())
			{
				outDestination = MessageTuple();
				return ErrorCode::NotFound;
			}

			outDestination = mQueue.front().second;

			return ErrorCode::OK;
		}

		ErrorCode InternalPop()
		{
			if (mThresholdMs > 0)
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
			assert(mThresholdMs > 0);

			const long long currentTimestampMs = fw::get_current_time();

			// Lock mQueue
			{
				std::lock_guard<std::mutex> lock(mMutex);

				int startSize = mSize;

				while (!mQueue.empty())
				{
					const long long createTimestampMs = mQueue.front().first;

#if WIN32
					if (std::llabs(currentTimestampMs - createTimestampMs) <= mThresholdMs)
						break;
#else
					if (abs(currentTimestampMs - createTimestampMs) <= mThresholdMs)
						break;
#endif

					mQueue.pop();
					mSize--;

					assert(mSize == static_cast<int>(mQueue.size()));
				}
			}
		}

		inline long long ConvertFpsToMs(float fps) const
		{
			assert(fps > 0.0F);
			return static_cast<long long>((1.0F / fps) * 1000.0F);
		}

		//inline long long GetTimestampMs() const
		//{
		//	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		//		std::chrono::system_clock::now().time_since_epoch());
		//
		//	return ms.count();
		//}

		std::mutex mMutex;
		std::queue<std::pair<long long, MessageTuple>> mQueue;

		std::string mName;

		int mSize = 0;
		int mBound = MAX_BOUND;

		float mSamplingFPS = MAX_SAMPLING_RATE_FPS;
		long long mSamplingMs = 1;

		long long mTimestampMs = 0;
		long long mThresholdMs = -1;
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