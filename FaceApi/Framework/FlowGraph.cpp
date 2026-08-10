#include "Framework/FlowGraph.hpp"

#include <future>
#include <thread>

namespace fw
{
  class InlineExecutor :
    public Executor
  {
    void run(std::function<void()> iTask) override
    {
      iTask();
    }
  };

  class ThreadExecutor :
    public Executor
  {
    void run(std::function<void()> iTask) override
    {
      std::thread(iTask).detach();
    }
  };

  Executor::Shared getInlineExecutor()
  {
    static Executor::Shared sExecutor = std::make_shared<InlineExecutor>();
    return sExecutor;
  }

  Executor::Shared getThreadExecutor()
  {
    static Executor::Shared sExecutor = std::make_shared<ThreadExecutor>();
    return sExecutor;
  }

  Continuation::Continuation(std::function<void(Executor::Shared)> iTask, unsigned iCounter) :
    mTask(iTask),
    mCounter(iCounter),
    mCount(iCounter)
  {
  }

  void Continuation::NotifyAndRun(Executor::Shared iExecutor)
  {
    if (--mCounter == 0U)
    {
      // Re-arm before running the task. The task may execute the whole downstream
      // graph synchronously (inline executor), and a notification arriving while
      // the counter still sits at 0 would underflow it and wedge this node.
      mCounter = mCount;

      auto task = mTask;

      iExecutor->run([iExecutor, task]
      {
        task(iExecutor);
      });
    }
  }
}
