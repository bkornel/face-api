#pragma once

// What every test file needs: a way to assert and a way to find the test assets.

#include <cstdio>
#include <filesystem>
#include <string>

namespace test
{
  inline int gFailures = 0;
  inline int gChecks = 0;

  inline void Check(bool iCondition, const std::string& iWhat)
  {
    ++gChecks;
    std::printf("%-70s %s\n", iWhat.c_str(), iCondition ? "ok" : "FAILED");
    if (!iCondition) ++gFailures;
  }

  inline void Near(double iValue, double iExpected, double iTolerance, const std::string& iWhat)
  {
    const bool ok = std::abs(iValue - iExpected) <= iTolerance;
    ++gChecks;

    std::printf("%-70s %s", iWhat.c_str(), ok ? "ok" : "FAILED");
    if (!ok) std::printf("  (%.4f, expected %.4f +- %.4f)", iValue, iExpected, iTolerance);
    std::printf("\n");

    if (!ok) ++gFailures;
  }

  inline void Section(const std::string& iName)
  {
    std::printf("\n-- %s\n", iName.c_str());
  }

  /// @brief The repository's Testing directory, found by walking up from the working
  /// directory - the test executable runs out of Bin/<Configuration>, which is nowhere
  /// near the assets. Empty when it cannot be found, which is a skip rather than a failure:
  /// the framework tests are worth running without the models.
  inline std::string FindTestRoot()
  {
    std::filesystem::path directory = std::filesystem::current_path();

    for (int depth = 0; depth < 8; ++depth)
    {
      const std::filesystem::path candidate = directory / "Testing";

      if (std::filesystem::exists(candidate / "configurations" / "settings.json") &&
          std::filesystem::exists(candidate / "media"))
      {
        return (candidate.string() + "/");
      }

      if (!directory.has_parent_path() || directory.parent_path() == directory) break;

      directory = directory.parent_path();
    }

    return {};
  }
}
