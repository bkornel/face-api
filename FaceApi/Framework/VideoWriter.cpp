#include "Framework/VideoWriter.h"
#include "Framework/Util.h"

#include <iostream>

namespace face
{
  const int VideoWriter::sDefaultFourCC = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');

  VideoWriter::~VideoWriter()
  {
    Close();
  }

  void VideoWriter::Create(const std::string& iPath, const std::string& iName, int iFourCC /*= sDefaultFourCC*/, double iFPS /*= 25.0*/, bool iIsColor /*= true*/)
  {
    Close();

    mFilename = iPath + iName + "_" + fw::get_log_stamp() + ".avi";
    mFourCC = iFourCC;
    mFPS = iFPS;
    mIsColor = iIsColor;
  }

  bool VideoWriter::Write(const cv::Mat& iFrame)
  {
    assert(!iFrame.empty());

    if (!IsOpened() && !mVideoWriter.open(mFilename, mFourCC, mFPS, iFrame.size(), mIsColor))
    {
      return false;
    }

    mVideoWriter << iFrame;

    return true;
  }

  void VideoWriter::Close()
  {
    if (IsOpened())
    {
      mVideoWriter.release();
    }
  }
}
