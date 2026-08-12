#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/FlowGraph.hpp"

#include <cstddef>
#include <memory>

namespace fw
{
  /// @brief A module's ports, reachable without knowing the signature behind them.
  ///
  /// fw::Port is a template, so every module has a different type for its ports and for the
  /// value they carry. Wiring the graph used to need that concrete type on both sides, which
  /// is why connecting one module meant walking a list of every module class there is.
  ///
  /// An output travels as fw::IFuture here. The input side knows what its port number expects
  /// and casts back to it, so a wrong connection is caught by the type the two ends actually
  /// disagree on, rather than by failing to guess the class of either.
  class IPortConnector
  {
  public:
    virtual ~IPortConnector() = default;

    /// @brief The output port, empty until Connect() has run
    virtual std::shared_ptr<IFuture> GetOutput() const = 0;

    /// @brief Points input port iIndex at iOutput.
    /// @return BadParam if the index is out of range, the port is already taken, iOutput is
    ///         empty, or iOutput carries something other than what this port expects
    virtual ErrorCode SetInput(std::size_t iIndex, const std::shared_ptr<IFuture>& iOutput) = 0;

    /// @brief Chooses where Main() runs. Has to be called before Connect().
    virtual void SetExecutor(const std::shared_ptr<Executor>& iExecutor) = 0;

    /// @brief Builds the output port from the inputs that have been set
    virtual ErrorCode Connect() = 0;

    virtual std::size_t GetInputCount() const = 0;
  };
}
