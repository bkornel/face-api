#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace fw
{
  namespace detail
  {
    template <typename T1, typename T2, typename std::enable_if<std::is_same<T1, T2>::value>::type* = nullptr>
    ErrorCode assign_input(const T1& iSource, T2& ioDestination)
    {
      ioDestination = iSource;
      return ErrorCode::OK;
    }

    template <typename T1, typename T2, typename std::enable_if<!std::is_same<T1, T2>::value>::type* = nullptr>
    ErrorCode assign_input(const T1& /*iSource*/, T2& /*ioDestination*/)
    {
      return ErrorCode::BadParam;
    }

    /// @brief Assigns iSource to the iIndex-th element of the ioDestination tuple.
    /// iIndex is a runtime value, so the slots are walked at compile time and only
    /// the matching one - if its type matches too - is assigned.
    /// Kept at namespace scope: an explicit specialization is not allowed in class scope.
    template <size_t S>
    struct input_port_setter
    {
      template <typename T1, typename T2>
      static ErrorCode Set(const T1& iSource, T2& ioDestination, size_t iIndex)
      {
        if (iIndex == (S - 1))
          return assign_input(iSource, std::get<S - 1>(ioDestination));

        return input_port_setter<S - 1>::Set(iSource, ioDestination, iIndex);
      }
    };

    template <>
    struct input_port_setter<0>
    {
      template <typename T1, typename T2>
      static ErrorCode Set(const T1& /*iSource*/, T2& /*ioDestination*/, size_t /*iIndex*/)
      {
        return ErrorCode::BadParam;
      }
    };
  } // namespace detail

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
  class Port<ReturnT(ArgumentT...)>
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
    virtual inline fw::ErrorCode Connect()
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

    template <typename T>
    inline fw::ErrorCode SetInputPort(T iValue, size_t iIndex)
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;

      if (iIndex >= size)
      {
        LOG(ERROR) << "Trying to set the input port no. " << iIndex << ", however the module has only " << size << " ports.";
        return fw::ErrorCode::BadParam;
      }

      if (mIsInputSet[iIndex])
      {
        LOG(ERROR) << "Input port " << iIndex << ", has already been set for the module.";
        return fw::ErrorCode::BadParam;
      }

      // An empty port would be indistinguishable from an input that was left out on
      // purpose, and the module would silently never run. It means the predecessor has
      // not been connected yet, i.e. the connection order is wrong.
      if (!iValue)
      {
        LOG(ERROR) << "Input port " << iIndex << " is set to an empty port, the predecessor is not connected yet.";
        return fw::ErrorCode::BadParam;
      }

      if (detail::input_port_setter<size>::Set(iValue, mInputPorts, iIndex) == fw::ErrorCode::OK)
      {
        mIsInputSet[iIndex] = true;
        return fw::ErrorCode::OK;
      }

      return fw::ErrorCode::BadParam;
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
    /// @brief Connects a source port: no dependency to wait for, so Trigger() drives it.
    inline fw::ErrorCode ConnectPort(std::true_type /*iIsSource*/)
    {
      auto source = fw::connect(FW_BIND(&Port::Main, this), mExecutor);

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
      mOutputPort = fw::connect(FW_BIND(&Port::Main, this), std::get<Is>(mInputPorts)...);
    }

    /// @brief Runs a source port's Main(). Downstream modules inherit the executor from
    /// the value the source publishes, so this single choice decides whether the whole
    /// graph runs inline or on separate threads.
    std::shared_ptr<Executor> mExecutor = fw::getInlineExecutor();

    std::function<void()> mTrigger = nullptr;
    OutputPort mOutputPort = nullptr;
    InputPorts mInputPorts;
    std::vector<bool> mIsInputSet;
  };
} // namespace fw
