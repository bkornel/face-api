#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/FlowGraph.hpp"
#include "Framework/Graph/IPortConnector.h"
#include "Framework/Graph/Module.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <memory>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace fw
{
  template <typename ReturnT>
  class Port;

  /// @brief The ports of a module: ArgumentT... are the input port types and ReturnT is
  /// the type published on the single output port.
  ///
  /// Inputs are optional. An input port that is never set is not a dependency, and its
  /// argument reaches Main() default-constructed - so a module can be wired up with only
  /// the predecessors it actually needs. Declaring no input at all makes the module a
  /// source: it has nothing to wait for and is driven by Trigger() instead.
  ///
  /// @param ReturnT return type of Main(), published on the output port
  /// @param ArgumentT input port types, in port order
  template <typename ReturnT, typename... ArgumentT>
  class Port<ReturnT(ArgumentT...)> : public IPortConnector
  {
  public:
    using OutputPort = fw::FutureShared<ReturnT>;
    using InputPorts = std::tuple<fw::FutureShared<ArgumentT>...>;

    Port()
    {
      mIsInputSet.resize(sizeof...(ArgumentT), false);
    }

    virtual ~Port() = default;

    virtual ReturnT Main(ArgumentT...) = 0;

    /// @brief Builds the output port from the input ports that have been set.
    /// A source port (no input declared) is driven by Trigger(); any other port needs at
    /// least one connected input, otherwise nothing would ever make it run.
    fw::ErrorCode Connect() override
    {
      return ConnectPort(std::integral_constant<bool, sizeof...(ArgumentT) == 0u>{});
    }

    /// @brief Runs a source port, i.e. one that declares no input port.
    inline void Trigger()
    {
      if (!mTrigger)
      {
        LOG(ERROR) << "Trigger() is only valid on a source port, i.e. one with no input port.";
        return;
      }

      mTrigger();
    }

    void SetExecutor(const std::shared_ptr<Executor>& iExecutor) override
    {
      if (iExecutor) mExecutor = iExecutor;
    }

    ErrorCode SetInput(std::size_t iIndex, const std::shared_ptr<IFuture>& iOutput) override
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;

      if (iIndex >= size)
      {
        LOG(ERROR) << "Trying to set the input port no. " << iIndex << ", however the module has only " << size << " ports.";
        return ErrorCode::BadParam;
      }

      if (mIsInputSet[iIndex])
      {
        LOG(ERROR) << "Input port " << iIndex << ", has already been set for the module.";
        return ErrorCode::BadParam;
      }

      // An empty port would be indistinguishable from an input that was left out on
      // purpose, and the module would silently never run. It means the predecessor has
      // not been connected yet, i.e. the connection order is wrong.
      if (!iOutput)
      {
        LOG(ERROR) << "Input port " << iIndex << " is set to an empty port, the predecessor is not connected yet.";
        return ErrorCode::BadParam;
      }

      return SetInput(iIndex, iOutput, std::make_index_sequence<size>{});
    }

    std::shared_ptr<IFuture> GetOutput() const override
    {
      return mOutputPort;
    }

    std::size_t GetInputCount() const override
    {
      return mIsInputSet.size();
    }

    inline std::size_t GetInputPortCount() const
    {
      return mIsInputSet.size();
    }

    inline std::size_t GetConnectedInputPortCount() const
    {
      std::size_t count = 0U;

      for (std::size_t i = 0U; i < mIsInputSet.size(); ++i)
        if (mIsInputSet[i]) count++;

      return count;
    }

    inline bool IsAllInputPortSet() const
    {
      return GetConnectedInputPortCount() == mIsInputSet.size();
    }

    inline OutputPort GetOutputPort() const
    {
      return mOutputPort;
    }

  protected:
    /// @brief Walks the ports at compile time and lets the one matching iIndex do the cast
    template <std::size_t... Is>
    ErrorCode SetInput(std::size_t iIndex, const std::shared_ptr<IFuture>& iOutput, std::index_sequence<Is...>)
    {
      ErrorCode result = ErrorCode::BadParam;
      ((iIndex == Is ? (void)(result = AssignInput<Is>(iOutput)) : void()), ...);
      return result;
    }

    /// @brief Casts iOutput to what port I expects. A mismatch here is a producer wired to a
    /// consumer that wants a different message, which is the error worth reporting.
    template <std::size_t I>
    ErrorCode AssignInput(const std::shared_ptr<IFuture>& iOutput)
    {
      using Expected = std::tuple_element_t<I, InputPorts>;

      auto typed = std::dynamic_pointer_cast<typename Expected::element_type>(iOutput);
      if (!typed)
      {
        LOG(ERROR) << "Input port " << I << " is connected to a port carrying a different type.";
        return ErrorCode::BadParam;
      }

      std::get<I>(mInputPorts) = typed;
      mIsInputSet[I] = true;

      return ErrorCode::OK;
    }

    // What FW_BIND used to build. connect() deduces ReturnT and ArgumentT... from the
    // std::function, so the type is spelled out here rather than deduced from a lambda.
    std::function<ReturnT(ArgumentT...)> MainAsFunction()
    {
      return [this](ArgumentT... iArgs) { return Main(iArgs...); };
    }

    /// @brief Connects a source port: no dependency to wait for, so Trigger() drives it.
    inline fw::ErrorCode ConnectPort(std::true_type /*iIsSource*/)
    {
      auto source = fw::connect(MainAsFunction(), mExecutor);

      mTrigger = source.first;
      mOutputPort = source.second;

      return mOutputPort ? fw::ErrorCode::OK : fw::ErrorCode::BadState;
    }

    /// @brief Connects a port that is run by its predecessors.
    inline fw::ErrorCode ConnectPort(std::false_type /*iIsSource*/)
    {
      if (GetConnectedInputPortCount() == 0U)
      {
        LOG(ERROR) << "None of the " << GetInputPortCount() << " input ports is connected, the module would never run.";
        return fw::ErrorCode::BadState;
      }

      // Unconnected inputs are legal, but they are far more often a configuration slip
      // than a deliberate choice, so say so out loud.
      if (!IsAllInputPortSet())
      {
        for (std::size_t i = 0U; i < mIsInputSet.size(); ++i)
          if (!mIsInputSet[i])
            LOG(WARNING) << "Input port " << i << " is not connected, Main() will receive an empty argument for it.";
      }

      static constexpr auto size = std::tuple_size<InputPorts>::value;
      ConnectPort(std::make_index_sequence<size>{});

      return mOutputPort ? fw::ErrorCode::OK : fw::ErrorCode::BadState;
    }

    template <size_t... Is>
    inline void ConnectPort(std::index_sequence<Is...> /*unused*/)
    {
      mOutputPort = fw::connect(MainAsFunction(), mExecutor, std::get<Is>(mInputPorts)...);
    }

    /// @brief Where this module's Main() runs. Per module, so putting one on the pool does not
    /// drag the rest of the graph off the calling thread with it.
    std::shared_ptr<Executor> mExecutor = fw::get_inline_executor();

    std::function<void()> mTrigger = nullptr;
    OutputPort mOutputPort = nullptr;
    InputPorts mInputPorts;
    std::vector<bool> mIsInputSet;
  };
} // namespace fw
