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
  
  FaceApp(const FaceApp& iOther) = delete;

  virtual ~FaceApp() = default;

  FaceApp& operator=(const FaceApp& iOther) = delete;

protected:
  int main(const std::vector<std::string>& args) override;

  void initialize(Poco::Util::Application& self) override;

  void uninitialize() override;

  void printProperties(const std::vector<std::string>& args);

  void printProperties(const std::string& base);

  void printKeys() const;

private:
  void showResults();

  bool mSaveVideo = false;

  cv::Mat mFrame;
  cv::Mat mResultFrame;
  face::VideoWriter mVideoWriter;
};
