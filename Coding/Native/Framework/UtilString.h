#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace fw
{
	namespace str
	{
		bool is_number(const std::string& value);

		bool convert_to_boolean(const std::string& value);

		bool ends_with(const std::string& str, const std::string& ending);

		bool starts_with(const std::string& str, const std::string& starting);

		std::string trim_left(const std::string& str);

		std::string trim_right(const std::string& str);

		std::string trim(const std::string& str);

		std::string to_upper(const std::string& str);

		std::string to_lower(const std::string& str);

		std::vector<std::string>& split(const std::string& str, char delim, std::vector<std::string>& elems);

		std::vector<std::string> split(const std::string& str, char delim);

		template<typename T>
		T convert_to_number(const std::string& value)
		{
			const std::string& trimmedValue = trim(value);
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
