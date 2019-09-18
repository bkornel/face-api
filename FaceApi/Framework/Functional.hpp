#pragma once

#include <functional>
#include <utility>

#define FW_BIND(function, callee) (fw::create_binder(function).template Create<function>(callee))

namespace fw
{
  template <int>
  struct variadic_placeholder
  {
  };
}

namespace std
{
  /// @brief partially specializations of std::is_placeholder for a custom template
  /// in order the generate placeholders via the int_sequence technique
  template <int N>
  struct is_placeholder<fw::variadic_placeholder<N>> :
    integral_constant<int, N + 1>
  {
  };
}

namespace fw
{
  /// @brief represents a compile-time sequence of integers
  template<int...>
  struct int_sequence {};

  /// @brief represents a compile-time sequence of integers
  template<int N, int... Is>
  struct make_int_sequence : make_int_sequence<N - 1, N - 1, Is...> {};

  /// @brief represents a compile-time sequence of integers
  template<int... Is>
  struct make_int_sequence<0, Is...> : int_sequence<Is...> {};

  /// @brief Helper class for creating variadic binders
  /// @param CalleeT type of the object whose member will be invoked
  /// @param ReturnT return type of the function that is being captured
  /// @param ArgumentT possible arguments of the captured function
  template<typename CalleeT, typename ReturnT, typename... ArgumentT>
  struct Binder
  {
    /// @brief BinderT alias for the binder type
    using BinderT = std::function<ReturnT(ArgumentT...)>;

    /// @brief Helper method of calling the captured function
    /// @param iCallee pointer to the object who's member will be called
    /// @param iArgument arguments of the invoked function
    /// @return the value of the captured member function
    template<ReturnT(CalleeT::*MemberFunction)(ArgumentT...)>
    static ReturnT MethodCaller(CalleeT* iCallee, ArgumentT... iArgument)
    {
      return (iCallee->*MemberFunction)(iArgument...);
    }

    /// @brief Creates the variadic binder through an integer sequence
    /// @param iCallee pointer to the object who's member will be called
    /// @param iSequence the placeholders
    /// @return the binder object (std::function)
    template<ReturnT(CalleeT::*MemberFunction)(ArgumentT...), int... Is>
    static BinderT Bind(CalleeT* iCallee, int_sequence<Is...> iSequence)
    {
      return std::bind(&MethodCaller<MemberFunction>, iCallee, variadic_placeholder<Is>{}...);
    }

    /// @brief Helper function for creating the variadic binder
    /// @param iCallee pointer to the object who's member will be called
    /// @return the binder object (std::function)
    template<ReturnT(CalleeT::*MemberFunction)(ArgumentT...)>
    static BinderT Create(CalleeT* iCallee)
    {
      auto placeholders = make_int_sequence<sizeof...(ArgumentT)>{};
      return Bind<MemberFunction>(iCallee, placeholders);
    }
  };

  /// @brief Helper function for creating binder objects
  template<typename CalleeT, typename ReturnT, typename... ArgumentT>
  static Binder<CalleeT, ReturnT, ArgumentT...> create_binder(ReturnT(CalleeT::* /*unused*/)(ArgumentT...))
  {
    return Binder<CalleeT, ReturnT, ArgumentT...>();
  }
}
