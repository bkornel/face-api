#include "Framework/Module.h"

namespace fw
{
	Module::CommandHandlerType Module::sCommandHandler;

	Module::Module(const std::string& iName) :
		Thread(),
		mName(iName)
	{
	}

	ErrorCode Module::Initialize(const cv::FileNode& iSettings)
	{
		ErrorCode result = ErrorCode::OK;

		if (!mInitialized)
		{
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
