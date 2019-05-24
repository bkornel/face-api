#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"
#include "Framework/UtilString.h"

#include <easyloggingpp/easyloggingpp.h>

#include <type_traits>
#include <utility>

namespace fw
{
  template<typename ReturnT>
  class Port;

  template<typename ReturnT, typename... ArgumentT>
  class Port<ReturnT(ArgumentT...)>
  {
    using OutputPort = fw::FutureShared<ReturnT>;
    using InputPorts = std::tuple<fw::FutureShared<ArgumentT>...>;

  public:
    Port()
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;

      if (size > 0u)
      {
        mIsInputSet.resize(size, false);
      }
    }

    virtual ~Port() = default;

    virtual ReturnT Main(ArgumentT...) = 0;

    virtual inline fw::ErrorCode Connect()
    {
      if (!IsAllInputPortSet())
      {
        return fw::ErrorCode::BadState;
      }

      static constexpr auto size = std::tuple_size<InputPorts>::value;
      Connect(std::make_index_sequence<size>{});

      return fw::ErrorCode::OK;
    }

    template<size_t index, typename T>
    inline fw::ErrorCode SetInputPort(T iValue)
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;

      if (mIsInputSet[index])
      {
        LOG(ERROR) << "Input port " << index << ", has already been set for the module.";
        return fw::ErrorCode::BadParam;
      }

      if (index >= size)
      {
        LOG(ERROR) << "Trying to set the input port no. " << index << ", however the module has only " << size << " ports.";
        return fw::ErrorCode::BadParam;
      }

      std::get<index>(mInputPorts) = iValue;
      mIsInputSet[index] = true;

      return fw::ErrorCode::OK;
    }

    inline bool IsAllInputPortSet() const
    {
      for (size_t i = 0u; i < mIsInputSet.size(); ++i)
        if (!mIsInputSet[i]) return false;

      return true;
    }

    inline OutputPort GetOutputPort() const
    {
      return mOutputPort;
    }

  protected:
    template<size_t... Is>
    inline void Connect(std::index_sequence<Is...>)
    {
      mOutputPort = fw::connect(FW_BIND(&Port::Main, this), std::get<Is>(mInputPorts)...);
    }

    OutputPort mOutputPort = nullptr;
    InputPorts mInputPorts;
    std::vector<bool> mIsInputSet;
  };
}
