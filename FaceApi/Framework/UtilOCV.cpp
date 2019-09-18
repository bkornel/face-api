#define _USE_MATH_DEFINES

#include "Framework/UtilOCV.h"

#include <opencv2/calib3d.hpp>

namespace fw
{
  namespace ocv
  {
    void rotate_mat(const cv::Mat& iInput, cv::Mat& iOutput, int iRotation)
    {
      if (iRotation == 90)				// transpose + flip(1) = CW
      {
        iOutput = iInput.t();			// Transpose original
        cv::flip(iOutput, iOutput, 1);	// Flipping around the y axis
      }
      else if (iRotation == 180)			// flip(-1) = 180
      {
        cv::flip(iInput, iOutput, -1);	// Flipping around both axis
      }
      else if (iRotation == 270)			// transpose + flip(0) = CCW
      {
        iOutput = iInput.t();			// Transpose original
        //cv::flip(output, output, 0);	// Flipping around the x axis
        cv::flip(iOutput, iOutput, -1);	// Flipping around both axis
      }
    }

    void put_text(const std::string& iText, const cv::Point& iLocation, cv::Mat& oFrame, bool iUseBG, const Font& iFont)
    {
      CV_DbgAssert(!oFrame.empty());

      if (iUseBG)
      {
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(iText, iFont.fontface, iFont.scale, iFont.thickness, &baseline);
        cv::Point pt(iLocation.x + 5, iLocation.y + 5);

        cv::rectangle(oFrame, pt + cv::Point(0, baseline), pt + cv::Point(textSize.width, -textSize.height), cv::Scalar::all(128), -1);
      }

      cv::putText(oFrame, iText, iLocation, iFont.fontface, iFont.scale, cv::Scalar::all(0.0), iFont.thickness + 1, iFont.lineType);
      cv::putText(oFrame, iText, iLocation, iFont.fontface, iFont.scale, cv::Scalar::all(255.0), iFont.thickness, iFont.lineType);
    }

    cv::Scalar get_color(double iV, double iMin, double iMax)
    {
      cv::Scalar c(1.0, 1.0, 1.0); // white

      if (iV < iMin) iV = iMin;
      if (iV > iMax) iV = iMax;

      double dv = (std::max)(iMax - iMin, std::numeric_limits<double>::epsilon());

      if (iV < (iMin + 0.25 * dv))
      {
        c[2] = 0.0;
        c[1] = 4.0 * (iV - iMin) / dv;
      }
      else if (iV < (iMin + 0.5 * dv))
      {
        c[2] = 0.0;
        c[0] = 1.0 + 4.0 * (iMin + 0.25 * dv - iV) / dv;
      }
      else if (iV < (iMin + 0.75 * dv))
      {
        c[2] = 4.0 * (iV - iMin - 0.5 * dv) / dv;
        c[0] = 0.0;
      }
      else
      {
        c[1] = 1.0 + 4.0 * (iMin + 0.75 * dv - iV) / dv;
        c[0] = 0.0;
      }

      return c * 255.0;
    }

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

    float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2)
    {
      if (iR1.area() == 0 || iR2.area() == 0)
        return 0.0F;

      const cv::Rect& intersection = (iR1 & iR2);
      return (std::max)((float)intersection.area() / (float)iR1.area(), (float)intersection.area() / (float)iR2.area());
    }

    double sum_squared(const cv::Mat& iMatrix)
    {
      CV_DbgAssert(!iMatrix.empty() && (iMatrix.channels() == 1 || iMatrix.channels() == 2 || iMatrix.channels() == 3));

      cv::Mat pow;
      cv::pow(iMatrix, 2.0, pow);

      const cv::Scalar& sum = cv::sum(pow);
      return sum[0] + sum[1] + sum[3];
    }

    cv::Mat get_camera_matrix(const cv::Size& iSize)
    {
      static cv::Size sImageSize(0, 0);
      static cv::Mat sCameraMatrix;

      if (sImageSize == iSize && !sCameraMatrix.empty())
        return sCameraMatrix;

      sImageSize = iSize;
#if 1
      const double dfov = FW_DEG_TO_RAD(70.0);
      const double d = std::sqrt(iSize.width*iSize.width + iSize.height*iSize.height);
      const double fd = (d / 2.0) / std::tan(dfov / 2.0);

      const double hfov = 2.0 * std::atan(iSize.width / (2.0 * fd));
      const double vfov = 2.0 * std::atan(iSize.height / (2.0 * fd));

      const double fx = (iSize.width / 2.0) / std::tan(hfov / 2.0);
      const double fy = (iSize.height / 2.0) / std::tan(vfov / 2.0);
#else
      // Approximate focal length, usually in range of 0.7*width <= fx <= width
      const double fx = size.width * 0.85;
      const double fy = fx;
#endif
      // principal point
      const cv::Point2d center((iSize.width - 1.0) * 0.5, (iSize.height - 1.0) * 0.5);

      // Camera matrix
      sCameraMatrix = cv::Mat_<double>({ 3, 3 }, {
        fx, 0.0, center.x,
        0.0, fy, center.y,
        0.0, 0.0, 1.0
        });

      return sCameraMatrix;
    }

    bool get_value(const cv::FileNode& iNode, const std::string& iName, std::string& oValue)
    {
      oValue = "";

      const cv::FileNode& subNode = iNode[iName.c_str()];

      if (!subNode.empty())
      {
        oValue = subNode.string();
        return !oValue.empty();
      }

      return false;
    }
  }
}
