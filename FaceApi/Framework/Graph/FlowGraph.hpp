#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace fw
{
  class Executor
  {
  public:
    virtual ~Executor() = default;

    virtual void Run(std::function<void()> iTask) = 0;
  };

  std::shared_ptr<Executor> get_inline_executor();

  /// @brief The process-wide worker pool. Bounded, so a graph that puts every module on it
  /// cannot create more threads than the machine can run.
  std::shared_ptr<Executor> get_thread_pool_executor();

  std::shared_ptr<Executor> make_thread_pool_executor(uint32_t iThreadCount);

  /// @brief Names an executor as it is written in the settings file. Unknown names fall back
  /// to the inline executor, which is what a graph gets when it asks for nothing.
  std::shared_ptr<Executor> get_executor_by_name(const std::string& iName);

  /// @brief A task that runs once all of its dependencies have reported in, on the executor it
  /// was built with. It re-arms itself afterwards, so the same node serves every frame.
  ///
  /// This class is thread-safe.
  class Continuation
  {
  public:
    Continuation(std::function<void()> iTask, int iCount, std::shared_ptr<Executor> iExecutor);

    void NotifyAndRun();

  private:
    const std::function<void()> mTask;
    const std::shared_ptr<Executor> mExecutor;
    const uint64_t mCount = 1ULL;

    std::atomic<uint64_t> mArrivals{ 0ULL };
  };

  template <typename T>
  class Promise;

  class IFuture
  {
  public:
    IFuture() = default;

    virtual ~IFuture() = default;

    virtual void Listen(std::shared_ptr<Continuation> iContinuation) = 0;

    virtual bool Ready() const = 0;

    virtual void Wait() const = 0;
  };

  /// @brief A flow graph equivalent of std::future, differing in that it notifies the
  /// continuations observing it once the promise puts a value.
  template <typename T>
  class Future : public IFuture
  {
    friend class Promise<T>;

  public:
    Future(const Future& iRhs) = delete;

    Future(Future&& iRhs) = default;

    ~Future() override = default;

    Future& operator=(const Future& iRhs) = delete;

    T Get() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      assert(mValue != nullptr);
      return mValue ? *mValue : T();
    }

    void Listen(std::shared_ptr<Continuation> iContinuation) override
    {
      bool isReady = false;
      {
        std::lock_guard<std::mutex> lock(mMutex);
        isReady = (mValue != nullptr);

        if (!isReady)
          mContinuations.emplace_back(iContinuation);
      }

      if (isReady)
        iContinuation->NotifyAndRun();
    }

    bool Ready() const override
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mValue != nullptr;
    }

    void Wait() const override
    {
      std::unique_lock<std::mutex> lock(mMutex);
      mCV.wait(lock, [this] { return mValue != nullptr; });
    }

    uint64_t GetGeneration() const
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mGeneration;
    }

    template <typename Rep, typename Period>
    bool WaitForNewValue(uint64_t iGeneration, const std::chrono::duration<Rep, Period>& iTimeout) const
    {
      std::unique_lock<std::mutex> lock(mMutex);
      return mCV.wait_for(lock, iTimeout, [this, iGeneration] { return mGeneration > iGeneration; });
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

    void Put(const T& iArg)
    {
      std::vector<std::shared_ptr<Continuation>> continuations;
      {
        std::lock_guard<std::mutex> lock(mMutex);
        mValue = std::make_unique<T>(iArg);
        ++mGeneration;
        continuations = mContinuations;
      }

      mCV.notify_all();

      for (const auto& continuation : continuations)
        continuation->NotifyAndRun();
    }

    mutable std::mutex mMutex;
    mutable std::condition_variable mCV;

    std::unique_ptr<T> mValue = nullptr;
    uint64_t mGeneration = 0ULL;
    std::vector<std::shared_ptr<Continuation>> mContinuations;
  };

  template <typename T>
  using FutureShared = std::shared_ptr<Future<T>>;

  template <typename T>
  class Promise
  {
  public:
    Promise() :
      mFuture(new Future<T>())
    {
    }

    Promise(const Promise<T>& iRhs) = delete;

    Promise(Promise<T>&& iRhs) = default;

    Promise<T>& operator=(const Promise<T>& iRhs) = delete;

    FutureShared<T> GetFuture() const
    {
      return mFuture;
    }

    void Put(const T& iArg)
    {
      mFuture->Put(iArg);
    }

  private:
    const FutureShared<T> mFuture = nullptr;
  };

  /// @brief The value of a Future, or a default-constructed one when the Future is absent.
  /// An absent Future is an input that was left unconnected.
  template <typename T>
  T get_or_default(const FutureShared<T>& iFuture)
  {
    return iFuture ? iFuture->Get() : T();
  }

  template <typename T>
  void listen_if_connected(const FutureShared<T>& iFuture, std::shared_ptr<Continuation> iContinuation)
  {
    if (iFuture) iFuture->Listen(iContinuation);
  }

  /// @brief Connects a source node: it has no dependency, so the returned action drives it.
  template <typename ReturnT>
  std::pair<std::function<void()>, FutureShared<ReturnT>> connect(std::function<ReturnT()> iFunction, std::shared_ptr<Executor> iExecutor)
  {
    // Held by shared_ptr because std::function is copyable and Promise is not
    auto promise = std::make_shared<Promise<ReturnT>>();
    auto future = promise->GetFuture();

    auto task = [iFunction, promise] {
      // See the sibling overload: publishing on failure keeps the graph's counters whole
      try
      {
        promise->Put(iFunction());
      }
      catch (...)
      {
        promise->Put(ReturnT{});
        throw;
      }
    };

    auto trigger = [task, iExecutor] {
      iExecutor->Run(task);
    };

    return std::make_pair(trigger, future);
  }

  /// @brief Connects a node run by its predecessors, on iExecutor once they are all ready.
  ///
  /// An absent Future in iFutures is an optional input: it never produces a value, so it is
  /// not counted as a dependency and its argument arrives default-constructed. Passing only
  /// absent Futures yields a Future that is never satisfied - callers must reject that case.
  template <typename ReturnT, typename... ArgumentT>
  FutureShared<ReturnT> connect(std::function<ReturnT(ArgumentT...)> iFunction, std::shared_ptr<Executor> iExecutor, FutureShared<ArgumentT>... iFutures)
  {
    auto promise = std::make_shared<Promise<ReturnT>>();
    auto future = promise->GetFuture();

    auto task = [iFunction, promise, iFutures...] {
      // A node that throws must still publish: its listeners count arrivals, and a missing
      // one would leave every downstream counter mid-round, firing them with the messages
      // of different frames from then on. The empty value degrades this frame instead.
      try
      {
        promise->Put(iFunction(get_or_default(iFutures)...));
      }
      catch (...)
      {
        promise->Put(ReturnT{});
        throw;
      }
    };

    const std::array<bool, sizeof...(ArgumentT)> connected = { { static_cast<bool>(iFutures)... } };
    const int count = static_cast<int>(std::count(connected.begin(), connected.end(), true));

    auto continuation = std::make_shared<Continuation>(std::move(task), count, std::move(iExecutor));

    (listen_if_connected(iFutures, continuation), ...);

    return future;
  }
}
