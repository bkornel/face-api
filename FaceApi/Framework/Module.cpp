#include "Framework/Module.h"

namespace fw
{
	Module::CommandHandlerType Module::commandHandler;

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
			commandHandler += MAKE_DELEGATE(&Module::OnCommandArrived, this);
		}

		return result;
	}

	ErrorCode Module::DeInitialize()
	{
		ErrorCode result = ErrorCode::OK;

		if (mInitialized)
		{
			Clear();

			if (IsRunning())
				result = StopThread();

			DeInitializeInternal();
			mInitialized = false;
			commandHandler -= MAKE_DELEGATE(&Module::OnCommandArrived, this);
		}

		return result;
	}
}