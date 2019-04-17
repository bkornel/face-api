#pragma once

#include <tuple>

namespace fw
{
	namespace tuple
	{
		template <size_t index>
		struct tuple_helper
		{
			template <typename TupleT, typename ValueT>
			static void set(TupleT& ioTuple, size_t iIndex, ValueT iValue)
			{
				if (iIndex == index - 1u)
				{
					std::get<index - 1>(ioTuple) = iValue;
				}
				else
				{
					tuple_helper<index - 1u>::set(ioTuple, iIndex, iValue);
				}
			}
		};

		template <>
		struct tuple_helper<0>
		{
			template <typename TupleT, typename ValueT>
			static void set(TupleT& /*ioTuple*/, size_t /*iIndex*/, ValueT /*iValue*/)
			{
				assert(false);
			}
		};

		template <typename ValueT, typename... Args>
		void set(std::tuple<Args...>& ioTuple, size_t iIndex, ValueT iValue)
		{
			tuple_helper<sizeof...(Args)>::set(ioTuple, iIndex, iValue);
		}
	}
}
