#include "Framework/Exception.h"

#include <sstream>

namespace fw
{
	Exception::Exception(ErrorCode iErrorCode, const std::string& iInfoText) :
		mErrorCode(iErrorCode)
	{
		CreateErrorText(iErrorCode, iInfoText);
	}

	Exception::Exception(ErrorCode iErrorCode, const char* iInfoText) :
		mErrorCode(iErrorCode)
	{
		CreateErrorText(iErrorCode, (iInfoText ? iInfoText : "empty"));
	}

	Exception::Exception(const Exception& iOther) :
		mErrorCode(iOther.mErrorCode),
		mErrorText(iOther.mErrorText)
	{
	}

	Exception::~Exception() noexcept
	{
	}

	Exception& Exception::operator=(const Exception& iOther)
	{
		if (this != &iOther)
		{
			mErrorCode = iOther.mErrorCode;
			mErrorText = iOther.mErrorText;
		}

		return *this;
	}

	void Exception::CreateErrorText(ErrorCode iErrorCode, const std::string& iInfoText)
	{
		std::stringstream ss;
		ss << "CODE: " << static_cast<int>(mErrorCode) << " INFO: " << (!iInfoText.empty() ? iInfoText : "empty");
		mErrorText = ss.str();
	}

	const char* Exception::what() const throw()
	{
		return mErrorText.c_str();
	}
}