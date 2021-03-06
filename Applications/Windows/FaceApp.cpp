﻿#include "FaceApp.h"
#include "FaceApi.h"

#include "Common/Configuration.h"
#include "Framework/UtilString.h"

#include <opencv2/imgcodecs.hpp>

void FaceApp::initialize(Application& self)
{
  static const cv::FileNode sSettingsNode;

  Poco::Util::Application::initialize(self);

  // add your own initialization code here
  Poco::Path path(commandPath());
  path = path.makeDirectory();
  path = path.popDirectory();
  path = path.pushDirectory("configurations");

  const std::string& workingDirectory = path.toString();
  face::FaceApi::GetInstance().SetWorkingDirectory(workingDirectory);
  face::FaceApi::GetInstance().Initialize(sSettingsNode);

  LOG(INFO) << name() << " starting up.";
  LOG(INFO) << name() << " working directory: " << workingDirectory;
}

void FaceApp::uninitialize()
{
  // add your own uninitialization code here
  LOG(INFO) << name() << " shutting down.";
  face::FaceApi::GetInstance().DeInitialize();
  Poco::Util::Application::uninitialize();
}

int FaceApp::main(const std::vector<std::string>& args)
{
  printProperties(args);

  if (args.empty())
  {
    LOG(ERROR) << "Usage: " << name() << " captureId";
    return Poco::Util::Application::EXIT_USAGE;
  }

  if (!face::FaceApi::GetInstance().IsInitialized())
  {
    LOG(ERROR) << name() << " is not initialized correctly.";
    return Poco::Util::Application::EXIT_USAGE;
  }

  cv::VideoCapture capture;
  if (fw::str::is_number(args[0]))
  {
    capture.open(fw::str::convert_to_number<int>(args[0]));
  }
  else
  {
    capture.open(args[0]);
  }

  if (!capture.isOpened())
  {
    LOG(ERROR) << "Capture was unable to open: " << args[0];
    return Poco::Util::Application::EXIT_USAGE;
  }

  printKeys();

  const auto& outputParams = face::Configuration::GetInstance().GetOutput();
  const auto& outputDir = face::Configuration::GetInstance().GetDirectories().output;
  mSaveVideo = outputParams.video;
  mVideoWriter.Create(outputDir, "sample", outputParams.videoFourCC, outputParams.videoFPS);

  while (capture.isOpened())
  {
    if (!face::FaceApi::GetInstance().IsRunning())
    {
      break;
    }

    capture >> mFrame;

    if (mFrame.empty())
    {
      break;
    }

    face::FaceApi::GetInstance().PushCameraFrame(mFrame);

    if (face::FaceApi::GetInstance().GetResultImage(mResultFrame) == fw::ErrorCode::OK)
    {
      showResults();

      if (mSaveVideo)
      {
        mVideoWriter.Write(mResultFrame);
      }
    }
  }

  return Poco::Util::Application::EXIT_OK;
}

void FaceApp::showResults()
{
  CV_DbgAssert(!mResultFrame.empty());

  cv::imshow(FW_PLUGIN_NAME, mResultFrame);
  const int keyPressed = cv::waitKey(1);

  if (keyPressed == 27)
  {
    LOG(INFO) << "Exiting from the application";
    face::FaceApi::GetInstance().StopThread();
  }
  else if (keyPressed == 's')
  {
    const std::string& name = "result_frame_" + std::to_string(face::FaceApi::GetInstance().GetLastFrameId()) + ".png";
    const std::string& path = face::Configuration::GetInstance().GetDirectories().output + name;
    cv::imwrite(path, mResultFrame);
    LOG(INFO) << "Saving frame to: " << path;
  }
  else if (keyPressed == 'c')
  {
    LOG(INFO) << "Clearing users";
    face::FaceApi::GetInstance().Clear();
  }
  else if (keyPressed == 'v')
  {
    LOG(INFO) << "Setting verbose mode ON/OFF";
    face::FaceApi::GetInstance().OnOffVerbose();
  }
  else if (keyPressed == 'd')
  {
    LOG(INFO) << "Forcing face detection";
    face::FaceApi::GetInstance().SetRunFaceDetector();
  }
  else if (keyPressed == 'o')
  {
    mSaveVideo = !mSaveVideo;
    LOG(INFO) << "Setting video output to: " << (mSaveVideo ? "ON" : "OFF");
  }
}

void FaceApp::printProperties(const std::vector<std::string>& args)
{
  LOG(INFO) << "Command line: ";
  std::ostringstream ostr;
  for (const auto & it : argv())
  {
    ostr << it << ' ';
  }
  LOG(INFO) << ostr.str();

  LOG(INFO) << "Arguments to main(): ";
  for (const auto & arg : args)
  {
    LOG(INFO) << arg;
  }

  LOG(INFO) << "Application properties: ";
  printProperties("");
}

void FaceApp::printProperties(const std::string& base)
{
  Poco::Util::AbstractConfiguration::Keys keys;
  config().keys(base, keys);

  if (keys.empty())
  {
    if (config().hasProperty(base))
    {
      std::string msg;
      msg.append(base);
      msg.append(" = ");
      msg.append(config().getString(base));
      LOG(INFO) << msg;
    }
  }
  else
  {
    for (auto & key : keys)
    {
      std::string fullKey = base;
      if (!fullKey.empty())
      {
        fullKey += '.';
      }
      fullKey.append(key);
      printProperties(fullKey);
    }
  }
}

void FaceApp::printKeys() const
{
  LOG(INFO) << "Registered keys:";
  LOG(INFO) << " - ESC: escape from the application";
  LOG(INFO) << " - s: save the current image";
  LOG(INFO) << " - c: clear all (users, cached images, etc.)";
  LOG(INFO) << " - v: verbose mode on/off";
  LOG(INFO) << " - o: force to write video on/off";
  LOG(INFO) << " - d: force to perform face detection";
}
