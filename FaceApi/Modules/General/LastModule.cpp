#include "Modules/General/LastModule.h"

namespace face
{
	bool LastModule::Main(bool iSucceeded)
	{
		return iSucceeded;
	}

	bool LastModule::Get() const 
	{ 
		return mOutputPort->Get();
	}

	void LastModule::Wait() const
	{ 
		mOutputPort->Wait();
	}
}
