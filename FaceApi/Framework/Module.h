#pragma once

#include <string>
#include <memory>
#include <opencv2/core.hpp>

#include "Framework/Event.hpp"
#include "Framework/Message.h"
#include "Framework/Thread.h"
#include "Framework/UtilOCV.h"
#include "Framework/UtilString.h"

namespace fw
{
	class Module :
		public Thread
	{
		using CommandHandlerType = Event<ErrorCode(Message::Shared)>;

	public:
		static CommandHandlerType commandHandler;

		explicit Module(const std::string& iName);

		virtual ~Module() = default;

		virtual ErrorCode Initialize(const cv::FileNode& iSettings);

		virtual ErrorCode DeInitialize();

		virtual void Clear()
		{
		}

		inline bool IsInitialized() const
		{
			return mInitialized;
		}

		inline const std::string& GetName() const
		{
			return mName;
		}

	protected:
		virtual ErrorCode OnCommandArrived(Message::Shared iMessage)
		{
			return fw::ErrorCode::OK;
		}

		virtual ErrorCode InitializeInternal(const cv::FileNode& iSettings)
		{
			return fw::ErrorCode::OK;
		}

		virtual ErrorCode DeInitializeInternal()
		{
			return fw::ErrorCode::OK;
		}

		bool mInitialized = false;

		std::string mName;
	};
}
