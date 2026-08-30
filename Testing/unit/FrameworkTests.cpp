// The framework primitives everything else stands on: the message queue's blocking calls,
// its sampler and Close(), the continuation's re-arm under contention, the thread pool, a
// small flow graph with the per-frame barrier, and parallel_for.
#include "TestSupport.h"

#include "Framework/Graph/FlowGraph.hpp"
#include "Framework/Graph/Port.hpp"
#include "Framework/Messaging/MessageQueue.hpp"
#include "Framework/Parallel.h"

#include <atomic>
#include <cstdio>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
  using test::Check;

  using Payload = std::shared_ptr<int>;
  using Queue = fw::MessageQueue<Payload>;

  void QueueBlockingPopIsWokenByPush()
  {
    Queue queue("q", 1000.0F, 8);

    std::atomic<bool> got{ false };
    std::thread consumer([&] {
      Payload out;
      if (queue.Pop(out) == fw::ErrorCode::OK && out && *out == 42)
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
      Payload out;
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

    Payload out;
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

  /// @brief Pushes iCount messages spaced iSpacing apart, paced by a spin wait so the
  /// spacing is exact to the microsecond, and pops after every push so the bound never
  /// interferes. Returns how many the sampler admitted.
  int PushPaced(Queue& ioQueue, int iCount, std::chrono::microseconds iSpacing)
  {
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();

    int admitted = 0;

    for (int i = 0; i < iCount; ++i)
    {
      while (Clock::now() < start + i * iSpacing)
      {
        // Spinning, not sleeping: the OS timer's 15 ms granularity would destroy the pacing
      }

      if (ioQueue.TryPush(std::make_shared<int>(i)) == fw::ErrorCode::OK) ++admitted;

      Payload out;
      ioQueue.TryPop(out);
    }

    return admitted;
  }

  void QueueSamplingKeepsAnAtRateSource()
  {
    // A 50 fps sampler fed at a shade over 50 fps, which is what a real camera at the
    // sampling rate looks like. The old arrival-to-arrival check landed every frame a hair
    // inside the period of the one before and turned away every second one.
    Queue queue("q", 50.0F, 8);

    constexpr int cFrames = 30;
    const int admitted = PushPaced(queue, cFrames, std::chrono::microseconds(19500));

    Check(admitted >= cFrames - 3, "a source at the sampling rate is not halved (" + std::to_string(admitted) + "/" + std::to_string(cFrames) + ")");
  }

  void QueueSamplingThrottlesAFastSource()
  {
    // The same sampler fed four times too fast must still be held to its rate
    Queue queue("q", 50.0F, 8);

    constexpr int cFrames = 40;
    const int admitted = PushPaced(queue, cFrames, std::chrono::microseconds(5000));

    Check(admitted >= cFrames / 5 && admitted <= cFrames / 2,
          "a source above the sampling rate is throttled (" + std::to_string(admitted) + "/" + std::to_string(cFrames) + ")");
  }

  void QueueRejectsWhenFullByDefault()
  {
    // No sampling limit: this is about what a full queue does, and a rate would turn the
    // back-to-back pushes away before they ever got there
    Queue queue("q");
    queue.SetBound(2);

    Check(queue.TryPush(std::make_shared<int>(1)) == fw::ErrorCode::OK, "first push onto a queue bounded at two");
    Check(queue.TryPush(std::make_shared<int>(2)) == fw::ErrorCode::OK, "second push fills it");
    Check(queue.TryPush(std::make_shared<int>(3)) == fw::ErrorCode::OutOfResources, "the third is turned away");

    Payload out;
    queue.TryPop(out);
    Check(out && *out == 1, "and the oldest message is still the one waiting");
  }

  void QueueDropsTheOldestWhenAskedTo()
  {
    // What an overlay wants: the graph is behind, and the freshest frame is the one worth
    // having rather than the one that has been waiting longest
    Queue queue("q");
    queue.SetBound(2);
    queue.SetDropPolicy(fw::DropPolicy::DropOldest);

    Check(queue.TryPush(std::make_shared<int>(1)) == fw::ErrorCode::OK, "first push onto a latest-wins queue");
    queue.TryPush(std::make_shared<int>(2));

    Check(queue.TryPush(std::make_shared<int>(3)) == fw::ErrorCode::OK, "a push onto a full latest-wins queue succeeds");
    Check(queue.GetSize() == 2, "the queue stays at its bound (" + std::to_string(queue.GetSize()) + ")");
    Check(queue.GetDroppedCount() == 1ULL, "and the overtaken message is counted as dropped");

    Payload first;
    Payload second;
    queue.TryPop(first);
    queue.TryPop(second);

    Check(first && *first == 2 && second && *second == 3, "what is left is the two newest messages");
  }

  void QueueClosedRefusesPushAndStillDrains()
  {
    Queue queue("q", 1000.0F, 8);
    queue.TryPush(std::make_shared<int>(7));
    queue.Close();

    Check(queue.TryPush(std::make_shared<int>(8)) == fw::ErrorCode::BadState, "a closed queue refuses a push");

    Payload out;
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

void RunFrameworkTests()
{
  test::Section("framework primitives");

  QueueBlockingPopIsWokenByPush();
  QueueCloseReleasesBlockedPop();
  QueueBlockingPushWaitsForRoom();
  QueueBlockingPushWaitsForExpiry();
  QueueSamplingKeepsAnAtRateSource();
  QueueSamplingThrottlesAFastSource();
  QueueRejectsWhenFullByDefault();
  QueueDropsTheOldestWhenAskedTo();
  QueueClosedRefusesPushAndStillDrains();
  ContinuationReArmsUnderContention();
  ThreadPoolRunsEveryTask();
  GraphRunsOnThePoolAndKeepsTheBarrier();
  ParallelForRunsEveryItemExactlyOnce();
  ParallelForSurvivesASaturatedPool();
  ParallelForRethrowsOnTheCaller();
  ParallelForWithoutExecutorRunsInline();
}
