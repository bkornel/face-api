#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fw
{
  namespace str
  {
    bool is_number(std::string_view iValue);

    bool convert_to_boolean(std::string_view iValue);

    std::string trim_left(std::string_view iString);

    std::string trim_right(std::string_view iString);

    std::string trim(std::string_view iString);

    std::string to_lower(std::string_view iString);

    std::vector<std::string>& split(const std::string& iString, char iDelimiter, std::vector<std::string>& iTokens);

    std::vector<std::string> split(const std::string& iString, char iDelimiter);

    // Returns T{} when iValue does not parse as a T. It used to return an uninitialized
    // local in that case, which read as a random threshold whenever a setting had a typo.
    template <typename T>
    T convert_to_number(const std::string& iValue)
    {
      std::stringstream ss(trim(iValue));

      T value{};
      ss >> value;

      return ss.fail() ? T{} : value;
    }
  }
}
