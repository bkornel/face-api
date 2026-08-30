#pragma once

#include <windows.h>

#include <cstddef>
#include <string>

namespace fe
{
  /// @brief The UTF conversions the engine needs at the Win32 boundary, in one place -
  /// they used to be copy-pasted per file.
  inline std::string ToUtf8(const std::wstring& iText)
  {
    if (iText.empty()) return {};

    const int size = WideCharToMultiByte(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
  }

  inline std::wstring ToWide(const std::string& iText)
  {
    if (iText.empty()) return {};

    const int size = MultiByteToWideChar(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()), nullptr, 0);
    if (size <= 0) return {};

    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, iText.c_str(), static_cast<int>(iText.size()), result.data(), size);
    return result;
  }
}
