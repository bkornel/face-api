#pragma once

#include "Framework/Util.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

namespace fw
{
  /// @ brief An interface for running tasks.
  class Executor
  {
  public:
    FW_DEFINE_SMART_POINTERS(Executor);

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
    FW_DEFINE_SMART_POINTERS(Continuation);

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

  class IFuture
  {
  public:
    FW_DEFINE_SMART_POINTERS(IFuture);

    IFuture() = default;

    virtual ~IFuture() = default;

    virtual void Listen(Continuation::Shared iContinuation) = 0;

    virtual bool Ready() const = 0;

    virtual void Wait() const = 0;
  };

  /// @brief Future is a flow graph equivalent of std::future. Future differs from a
  /// std::future in that it implements an observer pattern -- once the promise
  /// puts value, observing continuations are notified and run.
  template <typename T>
  class Future : public IFuture
  {
    friend class Promise<T>;

  public:
    Future(const Future& iRhs) = delete;

    Future(Future&& iRhs) = default;

    ~Future() override = default;

    Future& operator=(const Future& iRhs) = delete;

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
    void Listen(Continuation::Shared iContinuation) override
    {
      bool isValid = false;
      {
        std::lock_guard<std::mutex> lock(mMutex);
        isValid = (mValue != nullptr);

        if (!isValid)
          mContinuations.emplace_back(iContinuation);
      }

      if (isValid)
        iContinuation->NotifyAndRun(getInlineExecutor());
    }

    /// @brief ready returns true iff the value is ready.
    /// Subsequent calls to get() will  not block.
    bool Ready() const override
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mValue != nullptr;
    }

    /// @brief Blocks till this Future is ready.
    void Wait() const override
    {
      std::unique_lock<std::mutex> lock(mMutex);
      mCV.wait(lock, [this] { return mValue != nullptr; });
    }

    template <typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period>& iTimeout) const
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return mCV.wait_for(lock, iTimeout, [this] { return mValue != nullptr; });
    }

    template <typename Clock, typename Duration>
    bool WaitUntil(const std::chrono::time_point<Clock, Duration>& iTimeout) const
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return mCV.wait_until(lock, iTimeout, [this] { return mValue != nullptr; });
    }

  private:
    Future() = default;

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

    Promise(const Promise<T>& iRhs) = delete;

    Promise(Promise<T>&& iRhs) = default;

    Promise<T>& operator=(const Promise<T>& iRhs) = delete;

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
    const FutureShared<T> mFuture = nullptr;
  };

  /// @brief Returns an action running the provided function with executor and a Future for the result.
  /// This connect() is a special case, since no dependency is needed. It is provided for convenience.
  template <typename ReturnT, typename ArgumentT>
  std::pair<std::function<void()>, FutureShared<ReturnT>> connect(std::function<ReturnT(ArgumentT)> iFunction, Executor::Shared iExecutor)
  {
    // Using shared_ptr, because std::function is copyable, but Promise<R> is not.
    auto promise = std::make_shared<Promise<ReturnT>>();
    auto future = promise->GetFuture();

    auto task = [iFunction, promise, iExecutor] {
      ArgumentT arg{};
      promise->Put(iFunction(arg), iExecutor);
    };

    auto runTask = [task, iExecutor] {
      iExecutor->run(task);
    };

    return std::make_pair(runTask, future);
  }

  template <typename ReturnT, typename... ArgumentT>
  FutureShared<ReturnT> connect(std::function<ReturnT(ArgumentT...)> iFunction, FutureShared<ArgumentT>... iFutures)
  {
    // Using shared_ptr, because std::function is copyable, but Promise is not.
    auto promise = std::make_shared<Promise<ReturnT>>();
    auto future = promise->GetFuture();

    auto task = [iFunction, promise, iFutures...](Executor::Shared executor)
    {
      promise->Put(iFunction(iFutures->Get()...), executor);
    };

    const unsigned count = static_cast<unsigned>(sizeof...(ArgumentT));
    auto continuation = std::make_shared<Continuation>(std::move(task), count);

    // Expand it in the initializer
    int sf_array[] = {
      (void(iFutures->Listen(continuation)), 0)...
    };

    return future;
  }
}
