#pragma once

#include <vector>
#include <Poco/Util/Application.h>
#include <opencv2/core/core.hpp>

#include "Framework/VideoWriter.h"

class MainApp :
	public Poco::Util::Application
{
public:
	MainApp() = default;

	virtual ~MainApp() = default;

protected:
	virtual int main(const std::vector<std::string>& args);

	virtual void initialize(Poco::Util::Application& self);

	virtual void uninitialize();

	void printProperties(const std::vector<std::string>& args);

	void printProperties(const std::string& base);

	void printKeys() const;

private:
	MainApp(const MainApp&) = delete;
	MainApp& operator=(const MainApp&) = delete;

	void showResults();

	bool mSaveVideo = false;

	cv::Mat mFrame;
	cv::Mat mResultFrame;
	face::VideoWriter mVideoWriter;
};
