#pragma once

#include <Poco/Util/Application.h>

#include <opencv2/core/core.hpp>
#include <vector>

#include "FaceApi.h"
#include "Framework/Imaging/VideoWriter.h"

class FaceApp : public Poco::Util::Application
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

  void handleKey(int keyPressed);

  bool mSaveVideo = false;

  face::FaceApi mFaceApi;

  cv::Mat mFrame;
  cv::Mat mResultFrame;
  face::VideoWriter mVideoWriter;
};
