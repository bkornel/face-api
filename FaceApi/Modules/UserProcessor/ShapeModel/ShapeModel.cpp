#define _USE_MATH_DEFINES

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Modules/UserProcessor/ShapeModel/ShapeModel.h"
#include "Modules/UserProcessor/ShapeModel/ClmWrapper.h"

#include "Common/Configuration.h"
#include "Framework/UtilOCV.h"
#include "Framework/UtilString.h"

namespace face
{
  ShapeModel::ShapeModel()
  {
    mCLM = ClmWrapper::GetInstance().GetCLM();
    mFailureCheck = ClmWrapper::GetInstance().GetFailureCheck();
    mRefShape = ClmWrapper::GetInstance().GetReferenceShapeMat2D();
    mSimilarity = ClmWrapper::GetInstance().GetSimil();
    mNumberOfPts = mCLM._pdm.nPoints();
    mShape2D.create(2 * mNumberOfPts, 1, CV_64F);
    mCLM._pdm.Identity(mCLM._plocal, mCLM._pglobl);
  }

  void ShapeModel::InitShape(const cv::Rect& iRect)
  {
    const double a = iRect.width * cos(mSimilarity[1]) * mSimilarity[0] + 1.0;
    const double b = iRect.height * sin(mSimilarity[1]) * mSimilarity[0];

    const double tx = iRect.x + iRect.width / 2 + iRect.width * mSimilarity[2];
    const double ty = iRect.y + iRect.height / 2 + iRect.height * mSimilarity[3];

    auto sx = mRefShape.begin<double>();
    auto sy = mRefShape.begin<double>() + mNumberOfPts;
    auto dx = mShape2D.begin<double>();
    auto dy = mShape2D.begin<double>() + mNumberOfPts;

    for (int i = 0; i < mNumberOfPts; i++, ++sx, ++sy, ++dx, ++dy)
    {
      *dx = a * (*sx) - b * (*sy) + tx;
      *dy = b * (*sx) + a * (*sy) + ty;
    }

    mCLM._pdm.CalcParams(mShape2D, mCLM._plocal, mCLM._pglobl);
  }

  void ShapeModel::ShiftShape(const cv::Point& iOffset)
  {
    mCLM._pglobl.at<double>(4, 0) += iOffset.x;
    mCLM._pglobl.at<double>(5, 0) += iOffset.y;
  }

  void ShapeModel::Fit(const cv::Mat& iFrame, std::vector<int> &iWinSize, int iNoIter, double iClamp, double iFTol)
  {
    mCLM.Fit(iFrame, iWinSize, iNoIter, iClamp, iFTol);
    mCLM._pdm.CalcShape2D(mShape2D, mCLM._plocal, mCLM._pglobl);
  }

  bool ShapeModel::FailureCheck(const cv::Mat& iFrame)
  {
    return mFailureCheck.Check(mCLM.GetViewIdx(), iFrame, mShape2D);
  }

  bool ShapeModel::GetMinMax2D(const cv::Rect& iRect, cv::Point2d& oMin, cv::Point2d& oMax) const
  {
    auto x = mShape2D.begin<double>();
    auto y = mShape2D.begin<double>() + mNumberOfPts;

    oMin = oMax = { *x, *y };

    for (int i = 0; i < mNumberOfPts; i++)
    {
      const double vx = *x++;
      const double vy = *y++;

      oMin.x = (std::min)(oMin.x, vx);
      oMin.y = (std::min)(oMin.y, vy);
      oMax.x = (std::max)(oMax.x, vx);
      oMax.y = (std::max)(oMax.y, vy);
    }

    return (iRect.contains(oMin) && iRect.contains(oMax)) &&
      (!cvIsNaN(oMin.x) && !cvIsInf(oMin.x) && !cvIsNaN(oMin.y) && !cvIsInf(oMin.y) &&
        !cvIsNaN(oMax.x) && !cvIsInf(oMax.x) && !cvIsNaN(oMax.y) && !cvIsInf(oMax.y));
  }
}
