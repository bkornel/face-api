#pragma once

#include "Framework/VideoWriter.h"

#include <Poco/Util/Application.h>
#include <opencv2/core/core.hpp>
#include <vector>

class FaceApp :
  public Poco::Util::Application
{
public:
  FaceApp() = default;

  virtual ~FaceApp() = default;

protected:
  virtual int main(const std::vector<std::string>& args);

  virtual void initialize(Poco::Util::Application& self);

  virtual void uninitialize();

  void printProperties(const std::vector<std::string>& args);

  void printProperties(const std::string& base);

  void printKeys() const;

private:
  FaceApp(const FaceApp&) = delete;
  FaceApp& operator=(const FaceApp&) = delete;

  void showResults();

  bool mSaveVideo = false;

  cv::Mat mFrame;
  cv::Mat mResultFrame;
  face::VideoWriter mVideoWriter;
};
