#include "Framework/Ocv/Drawing.h"

#include <limits>

namespace fw
{
  namespace ocv
  {
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

      // Drawn twice: the thicker dark pass underneath keeps the text readable on any frame
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
  }
}
