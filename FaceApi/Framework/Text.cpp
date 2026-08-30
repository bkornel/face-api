#include "Framework/Text.h"

#include <algorithm>
#include <cctype>

namespace fw
{
  namespace str
  {
    static constexpr std::string_view sWhiteSpaces = " \n\r\t";

    bool is_number(std::string_view iValue)
    {
      return !iValue.empty() && std::all_of(iValue.begin(), iValue.end(), [](unsigned char c) {
               return std::isdigit(c) != 0;
             });
    }

    bool convert_to_boolean(std::string_view iValue)
    {
      const std::string trimmedValue = to_lower(trim(iValue));
      return trimmedValue == "true" || trimmedValue == "t" || trimmedValue == "1";
    }

    std::string trim_left(std::string_view iString)
    {
      const size_t startpos = iString.find_first_not_of(sWhiteSpaces);
      return (startpos == std::string_view::npos) ? "" : std::string(iString.substr(startpos));
    }

    std::string trim_right(std::string_view iString)
    {
      const size_t endpos = iString.find_last_not_of(sWhiteSpaces);
      return (endpos == std::string_view::npos) ? "" : std::string(iString.substr(0, endpos + 1));
    }

    std::string trim(std::string_view iString)
    {
      const size_t startpos = iString.find_first_not_of(sWhiteSpaces);
      if (startpos == std::string_view::npos) return "";

      const size_t endpos = iString.find_last_not_of(sWhiteSpaces);
      return std::string(iString.substr(startpos, endpos - startpos + 1));
    }

    std::string to_lower(std::string_view iString)
    {
      std::string result(iString);
      std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
      });

      return result;
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
  }
}
