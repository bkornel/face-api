#include "Framework/Graph/FlowGraph.hpp"
#include "Framework/Text.h"

#include <cstdint>
#include <queue>
#include <thread>

namespace fw
{
  namespace
  {
    class InlineExecutor : public Executor
    {
    public:
      void Run(std::function<void()> iTask) override
      {
        iTask();
      }
    };

    class ThreadPoolExecutor : public Executor
    {
    public:
      explicit ThreadPoolExecutor(uint32_t iThreadCount)
      {
        mThreads.reserve(iThreadCount);

        for (uint32_t i = 0U; i < iThreadCount; ++i)
          mThreads.emplace_back([this] { Work(); });
      }

      ~ThreadPoolExecutor() override
      {
        {
          std::lock_guard<std::mutex> lock(mMutex);
          mStopped = true;
        }

        mCV.notify_all();

        for (auto& thread : mThreads)
          if (thread.joinable()) thread.join();
      }

      void Run(std::function<void()> iTask) override
      {
        if (!iTask) return;

        {
          std::lock_guard<std::mutex> lock(mMutex);

          // Running it here would be the least surprising thing left to do: the pool is
          // going away and the caller still expects the task to have happened.
          if (mStopped)
          {
            iTask();
            return;
          }

          mTasks.emplace(std::move(iTask));
        }

        mCV.notify_one();
      }

    private:
      void Work()
      {
        for (;;)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mMutex);
            mCV.wait(lock, [this] { return mStopped || !mTasks.empty(); });

            if (mTasks.empty()) return;

            task = std::move(mTasks.front());
            mTasks.pop();
          }

          task();
        }
      }

      std::mutex mMutex;
      std::condition_variable mCV;
      std::queue<std::function<void()>> mTasks;
      std::vector<std::thread> mThreads;
      bool mStopped = false;
    };

    uint32_t DefaultThreadCount()
    {
      const uint32_t hardware = std::thread::hardware_concurrency();
      return hardware > 2U ? hardware - 1U : 2U;
    }

    std::shared_ptr<Executor> OrInline(std::shared_ptr<Executor> iExecutor)
    {
      return iExecutor ? std::move(iExecutor) : get_inline_executor();
    }
  }

  std::shared_ptr<Executor> get_inline_executor()
  {
    static const std::shared_ptr<Executor> sExecutor = std::make_shared<InlineExecutor>();
    return sExecutor;
  }

  std::shared_ptr<Executor> get_thread_pool_executor()
  {
    static const std::shared_ptr<Executor> sExecutor = std::make_shared<ThreadPoolExecutor>(DefaultThreadCount());
    return sExecutor;
  }

  std::shared_ptr<Executor> make_thread_pool_executor(uint32_t iThreadCount)
  {
    return std::make_shared<ThreadPoolExecutor>(iThreadCount > 0U ? iThreadCount : 1U);
  }

  std::shared_ptr<Executor> get_executor_by_name(const std::string& iName)
  {
    const std::string name = fw::str::to_lower(fw::str::trim(iName));

    if (name == "pool" || name == "threadpool" || name == "async")
      return get_thread_pool_executor();

    return get_inline_executor();
  }

  Continuation::Continuation(std::function<void()> iTask, int iCount, std::shared_ptr<Executor> iExecutor) :
    mTask(std::move(iTask)),
    mExecutor(OrInline(std::move(iExecutor))),
    mCount(iCount > 0 ? static_cast<uint64_t>(iCount) : 1ULL)
  {
  }

  void Continuation::NotifyAndRun()
  {
    // Arrivals are counted up and taken modulo the number of dependencies, rather than
    // counted down to zero and the count put back. Re-arming cannot be part of the same
    // atomic step as the decrement that completed the round, and the arrivals landing in
    // between drove such a counter past zero, from where it never came back.
    const uint64_t arrivals = mArrivals.fetch_add(1ULL, std::memory_order_acq_rel) + 1ULL;

    if (arrivals % mCount != 0ULL) return;

    mExecutor->Run(mTask);
  }
}
