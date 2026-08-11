#include "Framework/Ocv/Projection.h"

#include "Framework/MathExtensions.h"

#include <opencv2/calib3d.hpp>

namespace fw
{
  namespace ocv
  {
    void project_point(const cv::Point3d& iPoint3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, cv::Point2d& oPoint2D)
    {
      VectorPt3D points3D(1, iPoint3D);
      VectorPt2D points2D;

      project_point(points3D, iRvec, iTvec, iCameraMatrix, points2D);
      oPoint2D = points2D[0];
    }

    void project_point(const VectorPt3D& iPoints3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, VectorPt2D& oPoints2D)
    {
      static const cv::Mat sDistCoeffs = cv::Mat::zeros(4, 1, CV_64FC1);

      oPoints2D.clear();

      if (!iPoints3D.empty())
      {
        cv::projectPoints(iPoints3D, iRvec, iTvec, iCameraMatrix, sDistCoeffs, oPoints2D);
      }
    }

    cv::Mat get_camera_matrix(const cv::Size& iSize)
    {
      static cv::Size sImageSize(0, 0);
      static cv::Mat sCameraMatrix;

      if (sImageSize == iSize && !sCameraMatrix.empty())
        return sCameraMatrix;

      sImageSize = iSize;

      const double dfov = fw::deg_to_rad(70.0);
      const double d = std::sqrt(iSize.width * iSize.width + iSize.height * iSize.height);
      const double fd = (d / 2.0) / std::tan(dfov / 2.0);

      const double hfov = 2.0 * std::atan(iSize.width / (2.0 * fd));
      const double vfov = 2.0 * std::atan(iSize.height / (2.0 * fd));

      const double fx = (iSize.width / 2.0) / std::tan(hfov / 2.0);
      const double fy = (iSize.height / 2.0) / std::tan(vfov / 2.0);

      const cv::Point2d center((iSize.width - 1.0) * 0.5, (iSize.height - 1.0) * 0.5);

      sCameraMatrix = cv::Mat_<double>({ 3, 3 }, { fx, 0.0, center.x, 0.0, fy, center.y, 0.0, 0.0, 1.0 });

      return sCameraMatrix;
    }
  }
}
