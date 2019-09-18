#include "Framework/UtilString.h"

#include <algorithm>
#include <cctype>

namespace fw
{
  namespace str
  {
    static const std::string sWhiteSpaces = " \n\r\t";

    bool is_number(const std::string& iValue)
    {
      return !iValue.empty() && std::find_if(iValue.begin(), iValue.end(), [&](char c)
      {
        return !std::isdigit(c);
      }) == iValue.end();
    }

    bool convert_to_boolean(const std::string& iValue)
    {
      const std::string& trimmedValue = to_lower(trim(iValue));
      return trimmedValue == "true" || trimmedValue == "t" || trimmedValue == "1";
    }

    bool starts_with(const std::string& iString, const std::string& iStarting)
    {
      return !iStarting.empty() ? iString.substr(0, iStarting.size()) == iStarting : false;
    }

    bool ends_with(const std::string& iString, const std::string& iEnding)
    {
      return iString.length() >= iEnding.length() ? iString.compare(iString.length() - iEnding.length(), iEnding.length(), iEnding) == 0 : false;
    }

    std::string trim_left(const std::string& iString)
    {
      const size_t startpos = iString.find_first_not_of(sWhiteSpaces);
      return (startpos == std::string::npos) ? "" : iString.substr(startpos);
    }

    std::string trim_right(const std::string& iString)
    {
      const size_t endpos = iString.find_last_not_of(sWhiteSpaces);
      return (endpos == std::string::npos) ? "" : iString.substr(0, endpos + 1);
    }

    std::string trim(const std::string& iString)
    {
      return trim_right(trim_left(iString));
    }

    std::string to_upper(const std::string& iString)
    {
      if (iString.empty()) return "";

      std::string str2 = iString;
      std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);

      return str2;
    }

    std::string to_lower(const std::string& iString)
    {
      if (iString.empty()) return "";

      std::string str2 = iString;
      std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

      return str2;
    }

    std::vector<std::string>& split(const std::string& iString, char iDelimiter, std::vector<std::string>& iTokens)
    {
      std::stringstream ss(iString);
      std::string item;
      while (std::getline(ss, item, iDelimiter))
      {
        iTokens.emplace_back(item);
      }
      return iTokens;
    }

    std::vector<std::string> split(const std::string& iString, char iDelimiter)
    {
      std::vector<std::string> elems;
      split(iString, iDelimiter, elems);
      return elems;
    }

    Tokenizer::Tokenizer(const std::string& iString, const std::string& iDelimiters) :
      mOffset(0U),
      mString(iString),
      mDelimiters(iDelimiters)
    {
    }

    bool Tokenizer::NextToken()
    {
      const size_t i = mString.find_first_not_of(mDelimiters, mOffset);
      if (std::string::npos == i)
      {
        mOffset = mString.length();
        return false;
      }

      const size_t j = mString.find_first_of(mDelimiters, i);
      if (std::string::npos == j)
      {
        mToken = mString.substr(i);
        mOffset = mString.length();
        return true;
      }

      mToken = mString.substr(i, j - i);
      mOffset = j;

      return true;
    }
  }
}
