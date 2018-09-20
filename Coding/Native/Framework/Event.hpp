#pragma once

#include <algorithm>
#include <vector>

#define MAKE_DELEGATE(function, callee) (fw::MakeDelegate(function).Bind<function>(callee))

namespace fw
{
	/// @brief Delegate class for C++ (non specialized template declaration)
	/// Original implementation: http://marcmo.github.io/delegates/
	template<typename T>
	class Delegate;

	/// @brief Event class for C++ (non specialized template declaration)
	template<typename T>
	class Event;

	/// @brief Delegate class for C++ (specialization for member functions)
	/// @param ReturnT return type of the function that is being captured
	/// @param ArgumentT possible arguments of the captured function
	template<typename ReturnT, typename... ArgumentT>
	class Delegate<ReturnT(ArgumentT...)>
	{
		/// @brief FunctionT type of the member function
		using FunctionT = ReturnT(*)(void*, ArgumentT...);

		/// @brief DelegateT alias for the delegate type
		using DelegateT = Delegate<ReturnT(ArgumentT...)>;

	public:
		/// @brief Constructor
		/// @param iCallee pointer to the object who's member will be called
		/// @param iFunction pointer to the member function
		Delegate(void* iCallee, FunctionT iFunction) :
			mCallee(iCallee),
			mFunction(iFunction)
		{
		}

		/// @brief Destructor
		~Delegate() = default;

		/// @brief Ellipsis operator for invoking the delegate
		/// @param iArgument arguments of the invoked function
		/// @return the value that is determined in the invoked function
		ReturnT operator()(ArgumentT... iArgument) const
		{
			return (*mFunction)(mCallee, iArgument...);
		}

		/// @brief Operator for comparing two delegates
		/// @param iOther the delegate that is compared to the current instance
		/// @return true if the two delegates are equal; otherwise false
		bool operator==(const DelegateT& iOther) const
		{
			return (mCallee == iOther.mCallee) && (mFunction == iOther.mFunction);
		}

		/// @brief opposite of operator==
		bool operator!=(const DelegateT& iOther) const
		{
			return !(*this == iOther);
		}

	private:
		void* mCallee = nullptr;		///< pointer to the object who's member will be called
		FunctionT mFunction = nullptr;	///< pointer to the mCallee's member function
	};

	/// @brief Helper class for creating delegates
	/// @param CalleeT type of the object whose member will be invoked
	/// @param ReturnT return type of the function that is being captured
	/// @param ArgumentT possible arguments of the captured function
	template<typename CalleeT, typename ReturnT, typename... ArgumentT>
	struct DelegateMaker
	{
		/// @brief DelegateT alias for the delegate type
		using DelegateT = Delegate<ReturnT(ArgumentT...)>;

		/// @brief Helper method of calling the captured function
		/// @param iCallee pointer to the object who's member will be called
		/// @param iArgument arguments of the invoked function
		/// @return the value that is determined in the captured function
		template<ReturnT(CalleeT::*MemberFunction)(ArgumentT...)>
		static ReturnT MethodCaller(void* iCallee, ArgumentT... iArgument)
		{
			return (static_cast<CalleeT*>(iCallee)->*MemberFunction)(iArgument...);
		}

		/// @brief Helper function for binding the object's member function to the delegate
		/// @param iCallee pointer to the object who's member will be called
		/// @return the delegate itself
		template<ReturnT(CalleeT::*MemberFunction)(ArgumentT...)>
		static DelegateT Bind(CalleeT* iCallee)
		{
			return DelegateT(iCallee, &MethodCaller<MemberFunction>);
		}
	};

	/// @brief Helper function for creating delegates
	template<typename CalleeT, typename ReturnT, typename... ArgumentT>
	static DelegateMaker<CalleeT, ReturnT, ArgumentT... > MakeDelegate(ReturnT(CalleeT::*)(ArgumentT...))
	{
		return DelegateMaker<CalleeT, ReturnT, ArgumentT...>();
	}

	/// @brief Event class for C++ (specialization for member functions)
	/// @param ReturnT return type of the function that is being captured
	/// @param ArgumentT possible arguments of the captured function
	template<typename ReturnT, typename... ArgumentT>
	class Event<ReturnT(ArgumentT...)>
	{
		/// @brief DelegateT alias for the delegate type
		using DelegateT = Delegate<ReturnT(ArgumentT...)>;

	public:
		/// @brief Constructor
		Event() = default;

		/// @brief Destructor
		~Event() = default;

		/// @brief Raising an event
		/// @param iArgument possible arguments which will be forwarded to the captured function
		void Raise(ArgumentT... iArgument)
		{
			for (auto it = mDelegates.begin(); it != mDelegates.end(); ++it)
				(*it)(iArgument...);
		}

		/// @brief Unsubscribing from the event
		void Clear()
		{
			mDelegates.clear();
		}

		/// @brief Operator for subscribing to the event
		/// @param iDelegate the delegate that is invoked when the event is raised
		Event& operator+=(DelegateT iDelegate)
		{
			if (std::find(mDelegates.begin(), mDelegates.end(), iDelegate) == mDelegates.end())
				mDelegates.push_back(iDelegate);

			return *this;
		}

		/// @brief Operator for unsubscribing to the event
		/// @param iDelegate the delegate to be removed
		Event& operator-=(DelegateT iDelegate)
		{
			auto it = std::find(mDelegates.begin(), mDelegates.end(), iDelegate);
			if (it != mDelegates.end())
				mDelegates.erase(it);

			return *this;
		}

	private:
		std::vector<DelegateT> mDelegates;	///< Delegates that are subscribed to the current event
	};
}