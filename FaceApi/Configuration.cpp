#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include "Configuration.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>

#include <algorithm>

namespace face
{
  fw::ErrorCode Configuration::Initialize(const std::string& iWorkingDirectory, const std::string& iConfigFile)
  {
    // Start from the defaults, so a re-read does not inherit what a previous file set
    mModulesNode = cv::FileNode();
    mDirectories = DirectoryParams();
    mOutput = OutputParams();
    mVerbose = false;

    mDirectories.working = iWorkingDirectory;
    FixPathSeparator(mDirectories.working);

    const std::string jsonFile = mDirectories.working + iConfigFile;

    // Released explicitly: the nodes handed out above point into the open storage, and this
    // may well be a re-read - which is what a pipeline reload does
    mFileStorage.release();

    if (!mFileStorage.open(jsonFile, cv::FileStorage::READ))
    {
      LOG(ERROR) << "ERROR: " << jsonFile << " cannot be opened.";
      return fw::ErrorCode::BadData;
    }

    const cv::FileNode& faceNode = mFileStorage["face"];
    if (faceNode.empty())
    {
      LOG(ERROR) << "ERROR: " << jsonFile << " carries no <face> node.";
      return fw::ErrorCode::BadData;
    }

    const cv::FileNode& generalNode = faceNode["general"];
    if (!generalNode.empty())
    {
      LoadSettings(generalNode);

      const std::string& configFile = mDirectories.working + "log.cfg";
      const std::string& logFile = mDirectories.output + fw::get_log_stamp() + ".txt";

      el::Configurations conf(configFile);
      conf.set(el::Level::Global, el::ConfigurationType::Filename, logFile);
      el::Loggers::reconfigureAllLoggers(conf);
    }

    mModulesNode = faceNode["modules"];

    return fw::ErrorCode::OK;
  }

  bool Configuration::LoadSettings(const cv::FileNode& iGeneralNode)
  {
    std::string value;

    const cv::FileNode& outputNode = iGeneralNode["output"];
    if (!outputNode.empty())
    {
      if (fw::get_value(outputNode, "verbose", value))
        mVerbose = fw::str::convert_to_boolean(value);

      if (fw::get_value(outputNode, "video", value))
        mOutput.video = fw::str::convert_to_boolean(value);

      if (fw::get_value(outputNode, "videoFPS", value))
        mOutput.videoFPS = fw::str::convert_to_number<float>(value);

      if (fw::get_value(outputNode, "videoFourCC", value) && (value.size() == 4))
        mOutput.videoFourCC = cv::VideoWriter::fourcc(value[0], value[1], value[2], value[3]);
    }

    // The model files moved into the module settings; the only directory left to configure
    // is where the output goes
    const cv::FileNode& dirNode = iGeneralNode["directories"];
    if (!dirNode.empty())
    {
      if (fw::get_value(dirNode, "output", value))
        mDirectories.output = value;
    }

    mDirectories.output = mDirectories.working + mDirectories.output;
    FixPathSeparator(mDirectories.output);

    return true;
  }

  cv::FileNode Configuration::GetModuleSettings(const std::string& iName) const
  {
    static const cv::FileNode sEmptyNode;

    if (!mModulesNode.empty())
    {
      const std::string& lower1 = fw::str::to_lower(iName);

      for (const auto& m : mModulesNode)
      {
        if (m.empty() || !m.isNamed()) continue;

        const std::string& lower2 = fw::str::to_lower(m.name());

        if (lower1 == lower2) return m;
      }
    }

    LOG(WARNING) << "Settings for " << iName << " cannot be found.";

    return sEmptyNode;
  }

  void Configuration::FixPathSeparator(std::string& ioPath)
  {
    std::replace(ioPath.begin(), ioPath.end(), '\\', '/');
    if (!ioPath.empty() && !ioPath.ends_with("/"))
      ioPath += "/";
  }
}
