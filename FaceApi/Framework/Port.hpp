#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"
#include "Framework/UtilString.h"

#include <easyloggingpp/easyloggingpp.h>

#include <tuple>
#include <type_traits>
#include <utility>

namespace fw
{
  template<typename ReturnT>
  class Port;

  template<typename ReturnT, typename... ArgumentT>
  class Port<ReturnT(ArgumentT...)>
  {
  public:
    using OutputPort = fw::FutureShared<ReturnT>;
    using InputPorts = std::tuple<fw::FutureShared<ArgumentT>...>;

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

    template<typename T>
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

      if (InputPortHelper<size>::SetInputPort(iValue, mInputPorts, iIndex) == fw::ErrorCode::OK)
      {
        mIsInputSet[iIndex] = true;
        return fw::ErrorCode::OK;
      }

      return fw::ErrorCode::BadParam;
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
    template <size_t S>
    struct InputPortHelper
    {
      template<typename T1, typename T2>
      static fw::ErrorCode SetInputPort(const T1& iSource, T2& ioDestination, size_t iIndex)
      {
        if (iIndex == S - 1)
        {
          return SetInputPort(iSource, std::get<S - 1>(ioDestination));
        }

        return InputPortHelper<S - 1>::SetInputPort(iSource, ioDestination, iIndex);
      }

      template<typename T1, typename T2, typename std::enable_if<std::is_same<T1, T2>::value>::type* = nullptr>
      static fw::ErrorCode SetInputPort(const T1& iSource, T2& ioDestination)
      {
        ioDestination = iSource;
        return fw::ErrorCode::OK;
      }

      template<typename T1, typename T2, typename std::enable_if<!std::is_same<T1, T2>::value>::type* = nullptr>
      static fw::ErrorCode SetInputPort(const T1& iSource, T2& ioDestination)
      {
        CV_DbgAssert(false);
        return fw::ErrorCode::BadParam;
      }
    };

    template <>
    struct InputPortHelper<0>
    {
      template<typename T1, typename T2>
      static fw::ErrorCode SetInputPort(const T1& iSource, T2& ioDestination, size_t iIndex) 
      { 
        CV_DbgAssert(false);
        return fw::ErrorCode::BadParam;
      }
    };
    
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
