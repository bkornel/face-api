#pragma once

#include <opencv2/highgui/highgui.hpp>

namespace face
{
  class VideoWriter
  {
  public:
    const static int sDefaultFourCC;

    VideoWriter() = default;

    VideoWriter(const VideoWriter& iOther) = delete;

    ~VideoWriter();

    VideoWriter& operator=(const VideoWriter& iOther) = delete;

    void Create(const std::string& iPath, const std::string& iName, int iFourCC = sDefaultFourCC, double iFPS = 25.0, bool iIsColor = true);

    bool Write(const cv::Mat& iFrame);

    void Close();

    inline bool IsOpened() const
    {
      return mVideoWriter.isOpened();
    }

  private:
    cv::VideoWriter mVideoWriter;
    std::string mFilename;

    int mFourCC = sDefaultFourCC;
    double mFPS = 25.0;
    bool mIsColor = true;
  };
}
