#include "Framework/Exception.h"

#include <sstream>

namespace fw
{
  Exception::Exception(ErrorCode iErrorCode, const std::string& iInfoText) :
    mErrorCode(iErrorCode)
  {
    CreateErrorText(iInfoText);
  }

  Exception::Exception(ErrorCode iErrorCode, const char* iInfoText) :
    mErrorCode(iErrorCode)
  {
    CreateErrorText(iInfoText ? iInfoText : "empty");
  }

  Exception::Exception(const Exception& iOther) :
    mErrorCode(iOther.mErrorCode),
    mErrorText(iOther.mErrorText)
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

  void Exception::CreateErrorText(const std::string& iInfoText)
  {
    std::stringstream ss;
    ss << "CODE: " << static_cast<int>(mErrorCode) << " INFO: " << (!iInfoText.empty() ? iInfoText.c_str() : "empty");
    mErrorText = ss.str();
  }

  const char* Exception::what() const noexcept
  {
    return mErrorText.c_str();
  }
}
