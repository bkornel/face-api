#include "Framework/Module.h"

namespace fw
{
	Module::CommandHandlerType Module::sCommandHandler;

	ErrorCode Module::Initialize(const cv::FileNode& iSettings)
	{
		ErrorCode result = ErrorCode::OK;

		// Initialize only of it is not initialized
		if (!mInitialized)
		{
			// set the module name
			if (!iSettings.empty())
			{
				const cv::FileNode& nameNode = iSettings["name"];
				mName = !nameNode.empty() ? nameNode.name() : iSettings.name();
			}

			Clear();
			result = InitializeInternal(iSettings);
			mInitialized = (result == ErrorCode::OK);
			sCommandHandler += MAKE_DELEGATE(&Module::OnCommandArrived, this);
		}

		return result;
	}

	ErrorCode Module::DeInitialize()
	{
		ErrorCode result = ErrorCode::OK;

		if (mInitialized)
		{
			if (IsRunning())
			{
				result = StopThread();
			}

			DeInitializeInternal();
			Clear();

			mInitialized = false;
			sCommandHandler -= MAKE_DELEGATE(&Module::OnCommandArrived, this);
		}

		return result;
	}

	void Module::Clear()
	{
		// There is nothing to clear here, override the method in the child classes
	}

	ErrorCode Module::OnCommandArrived(Message::Shared iMessage)
	{
		return fw::ErrorCode::OK;
	}

	ErrorCode Module::InitializeInternal(const cv::FileNode& iSettings)
	{
		return fw::ErrorCode::OK;
	}

	ErrorCode Module::DeInitializeInternal()
	{
		return fw::ErrorCode::OK;
	}
}
