#include "Modules/General/LastModule.h"

namespace face
{
	bool LastModule::Main(bool iSucceeded)
	{
		return iSucceeded;
	}

	bool LastModule::Get() const 
	{ 
		return mPort->Get(); 
	}

	void LastModule::Wait() const
	{ 
		mPort->Wait(); 
	}
}
