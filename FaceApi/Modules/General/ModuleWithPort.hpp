#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"
#include "Framework/Tuple.hpp"

#include <easyloggingpp/easyloggingpp.h>

#include <type_traits>
#include <utility>

namespace face
{
  template<typename ReturnT>
  class ModuleWithPort;

  template<typename ReturnT, typename... ArgumentT>
  class ModuleWithPort<ReturnT(ArgumentT...)> :
    public fw::Module
  {
    using OutputPort = fw::FutureShared<ReturnT>;
    using InputPorts = std::tuple<fw::FutureShared<ArgumentT>...>;

  public:
    ModuleWithPort() = default;

    virtual ~ModuleWithPort() = default;

    virtual ReturnT Main(ArgumentT...) = 0;

    virtual void Connect()
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;
      connect(std::make_index_sequence<size>{});
    }

    inline OutputPort GetOutputPort() const
    {
      return mOutputPort;
    }

    template<size_t index, typename T>
    inline bool SetInputPort(T iValue)
    {
      static constexpr auto size = std::tuple_size<InputPorts>::value;

      if (index >= size)
      {
        LOG(ERROR) << "Trying to set the input port no. " << index << ", however the module " << GetName() << " has only " << size << " parameters.";
        return false;
      }

      std::get<index>(mInputPorts) = iValue;
      return true;
    }

    template<typename T>
    inline void SetInputPort_tmp(T iValue, size_t iIndex)
    {
      fw::tuple::set(mInputPorts, iIndex, iValue);
    }

  protected:
    template<size_t... Is>
    void connect(std::index_sequence<Is...>)
    {
      mOutputPort = fw::connect(FW_BIND(&ModuleWithPort::Main, this), std::get<Is>(mInputPorts)...);
    }

    OutputPort mOutputPort = nullptr;
    InputPorts mInputPorts;
  };
}
