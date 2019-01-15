#include "Module.h"

namespace fw
{
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
		}

		return result;
	}
}