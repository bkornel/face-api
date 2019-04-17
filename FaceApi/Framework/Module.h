#pragma once

#include <string>
#include <memory>
#include <opencv2/core.hpp>

#include "Framework/Event.hpp"
#include "Framework/FlowGraph.hpp"
#include "Framework/Message.h"
#include "Framework/Thread.h"

namespace fw
{
	class Module :
		public Thread
	{
	public:
		FW_DEFINE_SMART_POINTERS(Module)

		explicit Module(const std::string& iName);

		virtual ~Module() = default;

		virtual ErrorCode Initialize(const cv::FileNode& iSettings);

		virtual ErrorCode DeInitialize();

		// Should be deleted, now it's only called when the frame size has changed
		// This is going to be substituted with events
		virtual void Clear();

		inline bool IsInitialized() const
		{
			return mInitialized;
		}

		inline const std::string& GetName() const
		{
			return mName;
		}

	protected:
		using CommandHandlerType = Event<ErrorCode(Message::Shared)>;

		static CommandHandlerType sCommandHandler;

		virtual ErrorCode OnCommandArrived(Message::Shared iMessage);

		virtual ErrorCode InitializeInternal(const cv::FileNode& iSettings);

		virtual ErrorCode DeInitializeInternal();

		bool mInitialized = false;
		std::string mName;
	};

	template<typename ReturnT>
	class ModuleWithPort;

	template<typename ReturnT, typename... ArgumentT>
	class ModuleWithPort<ReturnT(ArgumentT...)> :
		public Module
	{
	public:
		explicit ModuleWithPort(const std::string& iName) :
			Module(iName)
		{
		}

		virtual ~ModuleWithPort() = default;

		virtual ReturnT Main(ArgumentT...) = 0;

		inline FutureShared<ReturnT> GetPort() const
		{
			return mPort;
		}
	
	// Set to protected later on
	//protected:
		FutureShared<ReturnT> mPort = nullptr;
		std::tuple<ArgumentT...> mArgs;
	};
}
