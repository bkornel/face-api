#pragma once

#include "Framework/Util.h"

#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>

#include <string>
#include <vector>

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
    std::string working;
    // Lower case on purpose: these are the real asset folder names, and Android
    // uses a case sensitive filesystem.
    std::string faceDetector = "facedetector/";
    std::string shapeModel = "shapemodel/";
    std::string output = "output/";
  };

  class Configuration
  {
  public:
    static Configuration& GetInstance();

    Configuration(const Configuration& iOther) = delete;

    Configuration& operator=(const Configuration& iOther) = delete;

    fw::ErrorCode Initialize(const std::string& iConfigFile = "settings.json");

    inline const DirectoryParams& GetDirectories() const { return mDirectories; }

    inline const OutputParams& GetOutput() const { return mOutput; }

    inline const cv::FileNode& GetModulesNode() const { return mModulesNode; }

    inline bool GetVerbose() const { return mVerbose; }

    cv::FileNode GetModuleSettings(const std::string& iName) const;

    void SetWorkingDirectory(const std::string& iWorkingDir);

  private:
    Configuration() = default;

    bool LoadSettings(const cv::FileNode& iGeneralNode);

    void RebuildPaths();

    void FixPathSeparator(std::string& ioPath);

    cv::FileStorage mFileStorage;
    cv::FileNode mModulesNode;

    /// @brief Paths relative to the working directory, as read from the settings.
    /// Kept separately so RebuildPaths() can always recompose the absolute paths
    /// from scratch instead of prepending the working directory to itself.
    DirectoryParams mRelativeDirectories;

    /// @brief Resolved absolute paths handed out by GetDirectories().
    DirectoryParams mDirectories;
    OutputParams mOutput;
    bool mVerbose = false;
  };
}
