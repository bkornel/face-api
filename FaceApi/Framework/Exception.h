#pragma once

#include <exception>
#include <string>

#include "Util.h"

namespace fw
{
	class Exception :
		public std::exception
	{
	public:
		Exception(ErrorCode iErrorCode, const std::string& iInfoText);

		Exception(ErrorCode iErrorCode, const char* iInfoText = "");

		Exception(const Exception& iOther);

		virtual ~Exception() noexcept;

		Exception& operator=(const Exception& iOther);

		virtual const char* what() const throw();

	private:
		void CreateErrorText(ErrorCode iErrorCode, const std::string& iInfoText);

		ErrorCode mErrorCode;
		std::string mErrorText;
	};
}
