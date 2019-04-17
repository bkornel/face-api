#include "Modules/General/FirstModule.h"
#include "Framework/Functional.hpp"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
	void FirstModule::Connect(fw::Executor::Shared iExecutor)
	{
		auto result = fw::connect(FW_BIND(&FirstModule::Main, this), iExecutor);
		mFunction = result.first;
		mOutputPort = result.second;
	}

	unsigned FirstModule::Main(bool)
	{
		static unsigned sTickCounter = 0U;
		return sTickCounter++;
	}

	void FirstModule::Tick()
	{
		mFunction();
	}

	void FirstModule::Connect()
	{
		LOG(WARNING) << "Do not call Connect() FirstModule. Call Connect(fw::Executor::Shared iExecutor) instead.";
	}
}
