#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Functional.hpp"
#include "Framework/Module.h"

#include <tuple>
#include <type_traits>
#include <utility>

namespace face
{
	//template <size_t... Is>
	//struct index_sequence;

	template<typename ReturnT>
	class ModuleWithPort;

	template<typename ReturnT, typename... ArgumentT>
	class ModuleWithPort<ReturnT(ArgumentT...)> :
		public fw::Module
	{
	public:
		ModuleWithPort() = default;

		virtual ~ModuleWithPort() = default;

		virtual ReturnT Main(ArgumentT...) = 0;

		virtual void Connect()
		{
			call(mArgs);
		}

		inline fw::FutureShared<ReturnT> GetPort() const
		{
			return mPort;
		}

		template<typename T>
		inline void SetArgument(T iValue, size_t iIndex)
		{
			auto setValue = [iValue](auto& ioElement) 
			{ 
				ioElement = iValue;
			};

			fw::tuple_at(mArgs, iIndex, setValue);
		}

		// TODO: Set to protected later on
		//protected:
		fw::FutureShared<ReturnT> mPort = nullptr;
		std::tuple<fw::FutureShared<ArgumentT>...> mArgs;

	private:
		template<typename Tuple, size_t... Is>
		void call(Tuple t, std::index_sequence<Is ...>)
		{
			// https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
			// http://aherrmann.github.io/programming/2016/02/28/unpacking-tuples-in-cpp14/
			//
			//mPort = fw::connect(FW_BIND(&ModuleWithPort::Main, this), std::get<Is>(t)...);
		}

		template<typename Tuple>
		auto call(Tuple t)
		{
			static constexpr auto size = std::tuple_size<Tuple>::value;
			call(t, std::make_index_sequence<size>{});
		}


		//template <size_t... Is>
		//void Connect2(index_sequence<Is...>)
		//{
		//	mImageQueue->mPort = fw::connect(FW_BIND(&ModuleWithPort::Main, this), std::get<Is>(mArgs)...);
		//}

	};
}
