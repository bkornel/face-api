#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "Framework/Util.h"

namespace fw
{
	/// @ brief An interface for running tasks.
	class Executor
	{
	public:
		FW_DEFINE_SMART_POINTERS(Executor)

		virtual ~Executor() = default;
		virtual void run(std::function<void()> iTask) = 0;
	};

	Executor::Shared getInlineExecutor();

	Executor::Shared getThreadExecutor();

	/// @brief Continuation is a task that waits for its dependencies to be ready.
	/// Providers of dependencies should call NotifyAndRun() once they are ready.
	/// This class is thread-safe.
	class Continuation
	{
	public:
		FW_DEFINE_SMART_POINTERS(Continuation)

		Continuation(std::function<void(Executor::Shared)> iTask, unsigned iCounter);

		/// @brief should be called once a dependency of this task is ready.
		/// The task runs, using executor, iff all dependencies are satisfied.
		/// The behavior of NotifyAndRun when called more times than there are
		/// dependencies is undefined.
		void NotifyAndRun(Executor::Shared iExecutor);

	private:
		std::function<void(Executor::Shared)> mTask;
		std::atomic_uint mCounter;
		const unsigned mCount = 0U;
	};

	template <typename T>
	class Promise;

	/// @brief Future is a flow graph equivalent of std::future. Future differs from a
	/// std::future in that it implements an observer pattern -- once the promise
	/// puts value, observing continuations are notified and run.
	template <typename T>
	class Future
	{
		friend class Promise<T>;

	public:
		~Future() = default;

		Future(Future&& iRhs) = default;

		/// @brief If ready, returns value stored in the future.Otherwise the behavior is undefined.
		T Get() const
		{
			std::lock_guard<std::mutex> lock(mMutex);
			return *mValue;
		}

		/// @brief If not ready, adds a continuation to be used once value is present.
		/// Added continuations will be notified once a value is put to this Future.
		///
		/// If ready, the continuation will be notified and run immediately with an
		/// inline executor.
		void Listen(Continuation::Shared iContinuation)
		{
			bool isValid = false;
			{
				std::lock_guard<std::mutex> lock(mMutex);
				isValid = (mValue.get() != nullptr);

				if (!isValid)
					mContinuations.push_back(iContinuation);
			}

			if (isValid)
				iContinuation->NotifyAndRun(getInlineExecutor());
		}

		/// @brief ready returns true iff the value is ready.
		/// Subsequent calls to get() will  not block.
		bool Ready() const
		{
			std::lock_guard<std::mutex> lock(mMutex);
			return (mValue.get() != nullptr);
		}

		/// @brief Blocks till this Future is ready.
		void Wait() const
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mCV.wait(lock, [this] { return (this->mValue.get() != nullptr); });
		}

		template <typename Rep, typename Period>
		bool WaitFor(const std::chrono::duration<Rep, Period>& iTimeout) const
		{
			std::unique_lock<std::mutex> lock(mMutex);
			return mCV.wait_for(lock, iTimeout, [this] { return (this->mValue.get() != nullptr); });
		}

		template <typename Clock, typename Duration>
		bool WaitUntil(const std::chrono::time_point<Clock, Duration>& iTimeout) const
		{
			std::unique_lock<std::mutex> lock(mMutex);
			return mCV.wait_until(lock, iTimeout, [this] { return (this->mValue.get() != nullptr); });
		}

	private:
		Future() = default;

		Future(const Future& iRhs) = delete;

		Future& operator=(const Future& iRhs) = delete;

		void Put(const T& iArg, Executor::Shared iExecutor)
		{
			std::vector<Continuation::Shared> continuationsAux;
			{
				std::unique_lock<std::mutex> lock(mMutex);
				mValue.reset(new T(iArg));
				continuationsAux = mContinuations;
			}

			mCV.notify_all();
			for (const auto& continuation : continuationsAux)
				continuation->NotifyAndRun(iExecutor);
		}

		mutable std::mutex mMutex;
		mutable std::condition_variable mCV;
		std::unique_ptr<T> mValue = nullptr;
		std::vector<Continuation::Shared>  mContinuations;
	};

	template <typename T>
	using FutureShared = std::shared_ptr<Future<T>>;

	/// @brief Promise is a flow graph equivalent of std::promise. 
	/// Future associated with the Promise.
	template <typename T>
	class Promise
	{
	public:
		Promise() : mFuture(new Future<T>()) {}

		Promise(Promise<T>&& iRhs) = default;

		FutureShared<T> GetFuture() const
		{
			return mFuture;
		}

		/// @brief Puts the value for the associated Future to return.Uses executor to run
		/// dependency tasks that are ready once after this put.
		///
		/// The behavior of put is undefined if this is called more than once.
		void Put(const T& iArg, Executor::Shared iExecutor)
		{
			mFuture->Put(iArg, iExecutor);
		}

	private:
		Promise(const Promise<T>& iRhs) = delete;
		Promise<T>& operator=(const Promise<T>& iRhs) = delete;

		const FutureShared<T> mFuture = nullptr;
	};

	template<typename T>
	struct Port
	{
		FutureShared<T> port = nullptr;
	};

	template <typename R>
	struct FirstNode
	{
		R Main() { static R sTickCounter = 0; return sTickCounter++; }
		void Tick() { port.first(); }
		std::pair<std::function<void()>, FutureShared<R>> port;
	};

	template <typename R>
	struct LastNode : public Port<R>
	{
		R Main(R iSucceeded) { return iSucceeded; }
		bool Get() { return Port<R>::port->Get(); }
		void Wait() { Port<R>::port->Wait(); }
	};

	/// @brief Returns an action running the provided function with executor and a Future for the result.
	/// This connect() is a special case, since no dependency is needed. It is provided for convenience.
	template <typename R>
	std::pair<std::function<void()>, FutureShared<R>> connect(std::function<R()> iFunction, Executor::Shared iExecutor)
	{
		// Using shared_ptr, because std::function is copyable, but Promise<R> is not.
		auto promise = std::make_shared<Promise<R>>();
		FutureShared<R> future = promise->GetFuture();

		auto task = [iFunction, promise, iExecutor] {
			promise->Put(iFunction(), iExecutor);
		};

		auto runTask = [task, iExecutor] {
			iExecutor->run(task);
		};

		return std::make_pair(runTask, future);
	}

	template <typename R, typename... Args>
	FutureShared<R> connect(std::function<R(Args...)> iFunction, FutureShared<Args>... iFutures)
	{
		// Using shared_ptr, because std::function is copyable, but Promise is not.
		auto promise = std::make_shared<Promise<R>>();
		auto future = promise->GetFuture();

		auto task = [iFunction, promise, iFutures...](Executor::Shared executor)
		{
			promise->Put(iFunction(iFutures->Get()...), executor);
		};

		const unsigned count = static_cast<unsigned>(sizeof...(Args));
		auto continuation = std::make_shared<Continuation>(std::move(task), count);

		// Expand it in the initializer
		int sf_array[] = {
			(void(iFutures->Listen(continuation)), 0)...
		};

		return future;
	}
}