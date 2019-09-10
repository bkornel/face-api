#include "Modules/UserProcessor/ShapeModel/ClmWrapper.h"

#include "Common/Configuration.h"
#include "Common/ShapeUtil.h"

namespace face
{
  ClmWrapper& ClmWrapper::GetInstance()
  {
    static ClmWrapper sInstance;
    return sInstance;
  }

  fw::ErrorCode ClmWrapper::Initialize(const std::string& iTrackerFile, const std::string& iTriFile, const std::string& iConFile)
  {
    if (mInitialized)
      return fw::ErrorCode::OK;

    fw::ErrorCode result = Load(iTrackerFile, iTriFile, iConFile);
    mInitialized = (result == fw::ErrorCode::OK);

    return result;
  }

  fw::ErrorCode ClmWrapper::Load(const std::string& iTrackerFile, const std::string& iTriFile, const std::string& iConFile)
  {
    const auto& path = Configuration::GetInstance().GetDirectories().shapeModel;

    cv::Mat trianglesMat = FACETRACKER::IO::LoadTri((path + iTriFile).c_str());
    CV_DbgAssert(!trianglesMat.empty());

    ShapeUtil::Triangles triangles;
    for (int i = 0; i < trianglesMat.rows; i++)
    {
      triangles.push_back(std::make_tuple(trianglesMat.at<int>(i, 0), trianglesMat.at<int>(i, 0), trianglesMat.at<int>(i, 2)));
    }
    ShapeUtil::GetInstance().SetTriangles(triangles);

    cv::Mat connectionsMat = FACETRACKER::IO::LoadCon((path + iConFile).c_str());
    CV_DbgAssert(!connectionsMat.empty());

    ShapeUtil::Connections connections;
    for (int i = 0; i < connectionsMat.cols; i++)
    {
      connections.push_back({ connectionsMat.at<int>(0, i), connectionsMat.at<int>(1, i) });
    }
    ShapeUtil::GetInstance().SetConnections(connections);

    std::ifstream infile(path + iTrackerFile);

    if (infile.is_open())
    {
      int type;
      infile >> type;
      CV_DbgAssert(type == FACETRACKER::IO::TRACKER);

      mCLM.Read(infile);
      mFailureCheck.Read(infile);

      FACETRACKER::IO::ReadMat(infile, mReferenceShapeMat2D);

      mCount = mReferenceShapeMat2D.rows / 2;
      mReferenceShape2D.resize(mCount);
      mReferenceShape3D.resize(mCount);

      infile >> mSimil[0] >> mSimil[1] >> mSimil[2] >> mSimil[3];
      mCLM._pdm.Identity(mCLM._plocal, mCLM._pglobl);

      const cv::Mat& refShape2D = mReferenceShapeMat2D;
      const cv::Mat& refShape3D = mCLM._pdm._M;
      CV_DbgAssert(mCount == (refShape3D.rows / 3));

      for (int i = 0; i < mCount; i++)
      {
        mReferenceShape2D[i] = { refShape2D.at<double>(i, 0), refShape2D.at<double>(i + mCount, 0) };
        mReferenceShape3D[i] = { refShape3D.at<double>(i, 0), refShape3D.at<double>(i + mCount, 0), refShape3D.at<double>(i + 2 * mCount, 0) };
      }

      return fw::ErrorCode::OK;
    }

    return fw::ErrorCode::BadData;
  }
}
