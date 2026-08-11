#include "Framework/FlowGraph.hpp"

#include <future>
#include <thread>

namespace fw
{
  class InlineExecutor : public Executor
  {
    void run(std::function<void()> iTask) override
    {
      iTask();
    }
  };

  class ThreadExecutor : public Executor
  {
    void run(std::function<void()> iTask) override
    {
      std::thread(iTask).detach();
    }
  };

  std::shared_ptr<Executor> getInlineExecutor()
  {
    static std::shared_ptr<Executor> sExecutor = std::make_shared<InlineExecutor>();
    return sExecutor;
  }

  std::shared_ptr<Executor> getThreadExecutor()
  {
    static std::shared_ptr<Executor> sExecutor = std::make_shared<ThreadExecutor>();
    return sExecutor;
  }

  Continuation::Continuation(std::function<void(std::shared_ptr<Executor>)> iTask, unsigned iCounter) :
    mTask(iTask),
    mCounter(iCounter),
    mCount(iCounter)
  {
  }

  void Continuation::NotifyAndRun(std::shared_ptr<Executor> iExecutor)
  {
    if (--mCounter == 0U)
    {
      // Re-arm before running: the inline executor runs the whole graph downstream.
      mCounter = mCount;

      auto task = mTask;

      iExecutor->run([iExecutor, task] {
        task(iExecutor);
      });
    }
  }
}
