#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace fw
{
  namespace str
  {
    bool is_number(const std::string& iValue);

    bool convert_to_boolean(const std::string& iValue);

    bool ends_with(const std::string& iString, const std::string& iEnding);

    bool starts_with(const std::string& iString, const std::string& iStarting);

    std::string trim_left(const std::string& iString);

    std::string trim_right(const std::string& iString);

    std::string trim(const std::string& iString);

    std::string to_upper(const std::string& iString);

    std::string to_lower(const std::string& iString);

    std::vector<std::string>& split(const std::string& iString, char iDelimiter, std::vector<std::string>& iTokens);

    std::vector<std::string> split(const std::string& iString, char iDelimiter);

    template<typename T>
    T convert_to_number(const std::string& iValue)
    {
      const std::string& trimmedValue = trim(iValue);
      std::stringstream ss(trimmedValue);

      T i;
      ss >> i;
      return i;
    }

    class Tokenizer
    {
    public:
      Tokenizer(const std::string& iString, const std::string& iDelimiters = " \t\n\r");

      bool NextToken();

      inline const std::string& GetToken() const
      {
        return mToken;
      }

    private:
      const std::string mString;
      const std::string mDelimiters;

      size_t mOffset = 0U;
      std::string mToken;
    };
  }
}
