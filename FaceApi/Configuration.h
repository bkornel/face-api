#pragma once


#include "Framework/ErrorCode.h"
#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>

#include <string>

namespace face
{
  struct OutputParams
  {
    bool video = false;
    float videoFPS = 15.0F;
    int videoFourCC = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  };

  struct DirectoryParams
  {
    /// @brief Where settings.json lives, and what every relative path resolves against
    std::string working;

    /// @brief Where logs, recordings and profiler dumps go
    std::string output = "output/";
  };

  /// @brief One settings.json, parsed. Owned by the FaceApi instance it configures - it
  /// used to be a process-wide singleton, which meant two pipelines could never read two
  /// different files.
  class Configuration
  {
  public:
    Configuration() = default;

    Configuration(const Configuration& iOther) = delete;

    Configuration& operator=(const Configuration& iOther) = delete;

    /// @brief Reads iConfigFile from iWorkingDirectory; may be called again to re-read
    fw::ErrorCode Initialize(const std::string& iWorkingDirectory, const std::string& iConfigFile = "settings.json");

    inline const DirectoryParams& GetDirectories() const
    {
      return mDirectories;
    }

    inline const OutputParams& GetOutput() const
    {
      return mOutput;
    }

    inline const cv::FileNode& GetModulesNode() const
    {
      return mModulesNode;
    }

    inline bool GetVerbose() const
    {
      return mVerbose;
    }

    cv::FileNode GetModuleSettings(const std::string& iName) const;

  private:
    bool LoadSettings(const cv::FileNode& iGeneralNode);

    static void FixPathSeparator(std::string& ioPath);

    cv::FileStorage mFileStorage;
    cv::FileNode mModulesNode;
    DirectoryParams mDirectories;
    OutputParams mOutput;
    bool mVerbose = false;
  };
}
