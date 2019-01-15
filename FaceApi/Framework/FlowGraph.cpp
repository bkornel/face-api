#include <future>
#include <thread>

#include "Framework/FlowGraph.hpp"

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
			auto task = mTask;

			iExecutor->run([iExecutor, task]
			{
				task(iExecutor);
			});

			mCounter = mCount;
		}
	}
}
