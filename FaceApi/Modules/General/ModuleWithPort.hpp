#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"
#include "Framework/Tuple.hpp"

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

		template<typename T>
		inline void SetInputPort(T iValue, size_t iIndex)
		{
			fw::tuple::set(mInputPorts, iIndex, iValue);
		}

		// TODO: Set to protected later on
		//protected:
		OutputPort mOutputPort = nullptr;
		InputPorts mInputPorts;

	private:
		template<size_t... Is>
		void connect(std::index_sequence<Is...>)
		{
			mOutputPort = fw::connect(FW_BIND(&ModuleWithPort::Main, this), std::get<Is>(mInputPorts)...);
		}
	};
}
