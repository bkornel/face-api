// Exercises the framework primitives everything else stands on: the message queue's
// blocking calls and Close(), the continuation's re-arm under contention, the thread
// pool, a small flow graph with the per-frame barrier, and parallel_for.
#include "Framework/Graph/FlowGraph.hpp"
#include "Framework/Graph/Port.hpp"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Parallel.h"

#include <easyloggingpp/easyloggingpp.h>

#include <atomic>
#include <cstdio>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

INITIALIZE_EASYLOGGINGPP

namespace
{
  int gFailures = 0;

  void Check(bool iCondition, const std::string& iWhat)
  {
    std::printf("%-62s %s\n", iWhat.c_str(), iCondition ? "ok" : "FAILED");
    if (!iCondition) ++gFailures;
  }

  using Payload = std::shared_ptr<int>;
  using Queue = fw::MessageQueue<Payload>;
  using Tuple = std::tuple<Payload>;

  void QueueBlockingPopIsWokenByPush()
  {
    Queue queue("q", 1000.0F, 8);

    std::atomic<bool> got{ false };
    std::thread consumer([&] {
      Tuple out;
      if (queue.Pop(out) == fw::ErrorCode::OK && std::get<0>(out) && *std::get<0>(out) == 42)
        got = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Push(std::make_shared<int>(42));

    consumer.join();
    Check(got.load(), "blocking Pop() is woken by a Push()");
  }

  void QueueCloseReleasesBlockedPop()
  {
    Queue queue("q", 1000.0F, 8);

    std::atomic<bool> released{ false };
    std::thread consumer([&] {
      Tuple out;
      released = (queue.Pop(out) == fw::ErrorCode::BadState);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Close();

    consumer.join();
    Check(released.load(), "Close() releases a thread blocked in Pop()");
  }

  void QueueBlockingPushWaitsForRoom()
  {
    Queue queue("q", 1000.0F, 1);
    Check(queue.TryPush(std::make_shared<int>(1)) == fw::ErrorCode::OK, "first push onto a queue bounded at one");
    Check(queue.TryPush(std::make_shared<int>(2)) == fw::ErrorCode::OutOfResources, "TryPush() on a full queue reports OutOfResources");

    std::atomic<bool> pushed{ false };
    std::thread producer([&] {
      pushed = (queue.Push(std::make_shared<int>(3)) == fw::ErrorCode::OK);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Check(!pushed.load(), "blocking Push() is still waiting while the queue is full");

    Tuple out;
    queue.TryPop(out);

    producer.join();
    Check(pushed.load(), "blocking Push() completes once a Pop() makes room");
  }

  void QueueBlockingPushWaitsForExpiry()
  {
    // Nothing pops here: the only thing that can make room is the message ageing out
    Queue queue("q", 1000.0F, 1, fw::Milliseconds(150.0));
    Check(queue.TryPush(std::make_shared<int>(1)) == fw::ErrorCode::OK, "push onto a queue that filters by timestamp");

    const auto start = std::chrono::steady_clock::now();
    const fw::ErrorCode code = queue.Push(std::make_shared<int>(2));
    const auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    Check(code == fw::ErrorCode::OK, "blocking Push() returns once the old message expires");
    Check(waited.count() >= 100 && waited.count() < 2000, "the wait was bounded by the expiry, not by a poll");
  }

  void QueueClosedRefusesPushAndStillDrains()
  {
    Queue queue("q", 1000.0F, 8);
    queue.TryPush(std::make_shared<int>(7));
    queue.Close();

    Check(queue.TryPush(std::make_shared<int>(8)) == fw::ErrorCode::BadState, "a closed queue refuses a push");

    Tuple out;
    Check(queue.TryPop(out) == fw::ErrorCode::OK, "a closed queue can still be drained");
    Check(queue.Pop(out) == fw::ErrorCode::BadState, "a drained closed queue reports BadState");
  }

  void ContinuationReArmsUnderContention()
  {
    // Two dependencies, many rounds, both arriving from separate threads at once: a re-arm
    // that is not one atomic step with the completion loses rounds here.
    constexpr int cRounds = 20000;

    std::atomic<int> runs{ 0 };
    auto continuation = std::make_shared<fw::Continuation>(
      [&runs] { ++runs; }, 2, fw::get_inline_executor());

    std::thread a([&] { for (int i = 0; i < cRounds; ++i) continuation->NotifyAndRun(); });
    std::thread b([&] { for (int i = 0; i < cRounds; ++i) continuation->NotifyAndRun(); });

    a.join();
    b.join();

    Check(runs.load() == cRounds, "a two-input continuation fires once per round (" + std::to_string(runs.load()) + "/" + std::to_string(cRounds) + ")");
  }

  void ThreadPoolRunsEveryTask()
  {
    auto pool = fw::make_thread_pool_executor(4U);

    std::atomic<int> done{ 0 };
    constexpr int cTasks = 500;

    for (int i = 0; i < cTasks; ++i)
      pool->Run([&done] { ++done; });

    for (int i = 0; i < 200 && done.load() < cTasks; ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    Check(done.load() == cTasks, "the thread pool ran every task (" + std::to_string(done.load()) + "/" + std::to_string(cTasks) + ")");
  }

  class Source : public fw::Port<int()>
  {
  public:
    int Main() override
    {
      return ++mCounter;
    }

  private:
    int mCounter = 0;
  };

  class Sink : public fw::Port<bool(int)>
  {
  public:
    bool Main(int iValue) override
    {
      mSeen = iValue;
      ++mCalls;
      return true;
    }

    std::atomic<int> mSeen{ 0 };
    std::atomic<int> mCalls{ 0 };
  };

  void GraphRunsOnThePoolAndKeepsTheBarrier()
  {
    auto source = std::make_shared<Source>();
    auto sink = std::make_shared<Sink>();

    source->SetExecutor(fw::get_inline_executor());
    sink->SetExecutor(fw::get_thread_pool_executor());

    Check(source->Connect() == fw::ErrorCode::OK, "the source port connects");
    Check(sink->SetInput(0U, source->GetOutput()) == fw::ErrorCode::OK, "the sink takes the source output");
    Check(sink->Connect() == fw::ErrorCode::OK, "the sink port connects");

    constexpr int cFrames = 200;
    int completed = 0;

    for (int i = 0; i < cFrames; ++i)
    {
      const uint64_t generation = sink->GetOutputPort()->GetGeneration();

      source->Trigger();

      if (sink->GetOutputPort()->WaitForNewValue(generation, fw::Milliseconds(2000.0)))
        ++completed;
    }

    Check(completed == cFrames, "every frame crossed the barrier (" + std::to_string(completed) + "/" + std::to_string(cFrames) + ")");
    Check(sink->mCalls.load() == cFrames, "the pooled module ran once per frame (" + std::to_string(sink->mCalls.load()) + "/" + std::to_string(cFrames) + ")");
    Check(sink->mSeen.load() == cFrames, "the pooled module saw the last value");
  }

  void ParallelForRunsEveryItemExactlyOnce()
  {
    constexpr std::size_t cCount = 10000U;
    std::vector<std::atomic<int>> hits(cCount);

    fw::parallel_for(cCount, [&](std::size_t i) { ++hits[i]; }, fw::get_thread_pool_executor());

    bool everyItemOnce = true;
    for (const auto& hit : hits)
      everyItemOnce = everyItemOnce && (hit.load() == 1);

    Check(everyItemOnce, "parallel_for ran every item exactly once");
  }

  void ParallelForSurvivesASaturatedPool()
  {
    // Every pool thread enters a parallel_for that fans out to the same pool. If the caller
    // did not drain the items itself, the waits would starve each other and never return.
    auto pool = fw::make_thread_pool_executor(2U);

    std::atomic<int> total{ 0 };
    std::atomic<int> finished{ 0 };
    constexpr int cOuter = 8;

    for (int i = 0; i < cOuter; ++i)
    {
      pool->Run([&] {
        fw::parallel_for(50U, [&](std::size_t) { ++total; }, pool);
        ++finished;
      });
    }

    for (int i = 0; i < 500 && finished.load() < cOuter; ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    Check(finished.load() == cOuter, "nested parallel_for on a saturated pool completes");
    Check(total.load() == cOuter * 50, "and ran every item (" + std::to_string(total.load()) + "/" + std::to_string(cOuter * 50) + ")");
  }

  void ParallelForRethrowsOnTheCaller()
  {
    bool rethrown = false;

    try
    {
      fw::parallel_for(64U, [](std::size_t i) {
        if (i == 13U) throw std::runtime_error("boom");
      }, fw::get_thread_pool_executor());
    }
    catch (const std::runtime_error&)
    {
      rethrown = true;
    }

    Check(rethrown, "an exception in an item is rethrown on the caller");
  }

  void ParallelForWithoutExecutorRunsInline()
  {
    const auto callerId = std::this_thread::get_id();
    bool allInline = true;

    fw::parallel_for(16U, [&](std::size_t) {
      allInline = allInline && (std::this_thread::get_id() == callerId);
    }, nullptr);

    Check(allInline, "without an executor every item runs on the caller");
  }
}

int main()
{
  QueueBlockingPopIsWokenByPush();
  QueueCloseReleasesBlockedPop();
  QueueBlockingPushWaitsForRoom();
  QueueBlockingPushWaitsForExpiry();
  QueueClosedRefusesPushAndStillDrains();
  ContinuationReArmsUnderContention();
  ThreadPoolRunsEveryTask();
  GraphRunsOnThePoolAndKeepsTheBarrier();
  ParallelForRunsEveryItemExactlyOnce();
  ParallelForSurvivesASaturatedPool();
  ParallelForRethrowsOnTheCaller();
  ParallelForWithoutExecutorRunsInline();

  std::printf("\n%s (%d failure(s))\n", gFailures == 0 ? "PASS" : "FAIL", gFailures);
  return gFailures == 0 ? 0 : 1;
}
