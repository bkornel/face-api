#include "Framework/UtilString.h"

#include <algorithm>
#include <cctype>

namespace fw
{
  namespace str
  {
    static const std::string sWhiteSpaces = " \n\r\t";

    bool is_number(const std::string& value)
    {
      return !value.empty() && std::find_if(value.begin(), value.end(), [&](char c)
      {
        return !std::isdigit(c);
      }) == value.end();
    }

    bool convert_to_boolean(const std::string& value)
    {
      const std::string& trimmedValue = to_lower(trim(value));
      return trimmedValue == "true" || trimmedValue == "t" || trimmedValue == "1";
    }

    bool starts_with(const std::string& str, const std::string& starting)
    {
      return !starting.empty() ? str.substr(0, starting.size()) == starting : false;
    }

    bool ends_with(const std::string& str, const std::string& ending)
    {
      return str.length() >= ending.length() ? str.compare(str.length() - ending.length(), ending.length(), ending) == 0 : false;
    }

    std::string trim_left(const std::string& str)
    {
      const size_t startpos = str.find_first_not_of(sWhiteSpaces);
      return (startpos == std::string::npos) ? "" : str.substr(startpos);
    }

    std::string trim_right(const std::string& str)
    {
      const size_t endpos = str.find_last_not_of(sWhiteSpaces);
      return (endpos == std::string::npos) ? "" : str.substr(0, endpos + 1);
    }

    std::string trim(const std::string& str)
    {
      return trim_right(trim_left(str));
    }

    std::string to_upper(const std::string& str)
    {
      if (str.empty()) return "";

      std::string str2 = str;
      std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);

      return str2;
    }

    std::string to_lower(const std::string& str)
    {
      if (str.empty()) return "";

      std::string str2 = str;
      std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

      return str2;
    }

    std::vector<std::string>& split(const std::string& str, char delim, std::vector<std::string>& elems)
    {
      std::stringstream ss(str);
      std::string item;
      while (std::getline(ss, item, delim))
      {
        elems.push_back(item);
      }
      return elems;
    }

    std::vector<std::string> split(const std::string& str, char delim)
    {
      std::vector<std::string> elems;
      split(str, delim, elems);
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
