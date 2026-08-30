#include "Framework/Parallel.h"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace fw
{
  namespace
  {
    // Shared by the caller and the offloaded workers. Owned by shared_ptr: a worker that the
    // pool only gets to after the call returned still finds valid state, sees no items left
    // and exits without touching anything else.
    struct ParallelState
    {
      std::function<void(std::size_t)> function;
      std::size_t count = 0U;

      std::atomic<std::size_t> next{ 0U };
      std::atomic<int> active{ 0 };

      std::mutex mutex;
      std::condition_variable done;
      std::exception_ptr error;

      void Drain()
      {
        for (;;)
        {
          const std::size_t index = next.fetch_add(1U, std::memory_order_relaxed);
          if (index >= count) return;

          try
          {
            function(index);
          }
          catch (...)
          {
            std::lock_guard<std::mutex> lock(mutex);
            if (!error) error = std::current_exception();
          }
        }
      }
    };
  }

  void parallel_for(std::size_t iCount, const std::function<void(std::size_t)>& iFunction, const std::shared_ptr<Executor>& iExecutor)
  {
    if (iCount == 0U || !iFunction) return;

    // Nothing to share out: run in place and let an exception travel as usual
    if (iCount == 1U || !iExecutor)
    {
      for (std::size_t i = 0U; i < iCount; ++i)
        iFunction(i);

      return;
    }

    auto state = std::make_shared<ParallelState>();
    state->function = iFunction;
    state->count = iCount;

    const std::size_t hardware = (std::max)(std::thread::hardware_concurrency(), 2U);
    const std::size_t workers = (std::min)(iCount - 1U, hardware - 1U);

    for (std::size_t i = 0U; i < workers; ++i)
    {
      iExecutor->Run([state] {
        // Counted only once actually running: a worker the pool never starts must not be
        // waited for, the caller drains the items itself in that case.
        state->active.fetch_add(1, std::memory_order_acquire);
        state->Drain();

        if (state->active.fetch_sub(1, std::memory_order_release) == 1)
        {
          std::lock_guard<std::mutex> lock(state->mutex);
          state->done.notify_all();
        }
      });
    }

    state->Drain();

    // Every item has been taken; only workers still finishing their last one are left
    std::unique_lock<std::mutex> lock(state->mutex);
    state->done.wait(lock, [&state] { return state->active.load(std::memory_order_acquire) == 0; });

    if (state->error) std::rethrow_exception(state->error);
  }
}
