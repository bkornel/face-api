#pragma once

#include "Framework/Graph/FlowGraph.hpp"

#include <cstddef>
#include <functional>
#include <memory>

namespace fw
{
  /// @brief Runs iFunction(0..iCount-1), sharing the items between the calling thread and
  /// iExecutor, and returns once every item has run.
  ///
  /// The caller always works too: it keeps taking items until none are left, so the call
  /// completes even if the executor never picks anything up. That is what makes it safe to
  /// use from a task that itself runs on the pool - a full pool degrades to serial instead
  /// of deadlocking. With an empty executor the items simply run on the caller.
  ///
  /// An exception from an item is rethrown on the calling thread once everything finished.
  void parallel_for(std::size_t iCount,
                    const std::function<void(std::size_t)>& iFunction,
                    const std::shared_ptr<Executor>& iExecutor);
}
