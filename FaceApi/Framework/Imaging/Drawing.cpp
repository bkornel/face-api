#include "Framework/Imaging/Drawing.h"

#include <algorithm>
#include <limits>

namespace fw
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

  void fill_rounded_rect(cv::Mat& ioFrame, const cv::Rect& iRect, const cv::Scalar& iColor, int iRadius)
  {
    if (iRect.width <= 0 || iRect.height <= 0) return;

    const int radius = (std::max)(0, (std::min)(iRadius, (std::min)(iRect.width, iRect.height) / 2));

    if (radius == 0)
    {
      cv::rectangle(ioFrame, iRect, iColor, -1, cv::LINE_AA);
      return;
    }

    // A cross of two rectangles plus a disc in each corner
    cv::rectangle(ioFrame, cv::Rect(iRect.x + radius, iRect.y, iRect.width - 2 * radius, iRect.height), iColor, -1, cv::LINE_AA);
    cv::rectangle(ioFrame, cv::Rect(iRect.x, iRect.y + radius, iRect.width, iRect.height - 2 * radius), iColor, -1, cv::LINE_AA);

    const int right = iRect.x + iRect.width - radius - 1;
    const int bottom = iRect.y + iRect.height - radius - 1;

    cv::circle(ioFrame, { iRect.x + radius, iRect.y + radius }, radius, iColor, -1, cv::LINE_AA);
    cv::circle(ioFrame, { right, iRect.y + radius }, radius, iColor, -1, cv::LINE_AA);
    cv::circle(ioFrame, { iRect.x + radius, bottom }, radius, iColor, -1, cv::LINE_AA);
    cv::circle(ioFrame, { right, bottom }, radius, iColor, -1, cv::LINE_AA);
  }

  void blend_rounded_rect(cv::Mat& ioFrame, const cv::Rect& iRect, const cv::Scalar& iColor, int iRadius, double iAlpha)
  {
    const cv::Rect roi = iRect & cv::Rect(0, 0, ioFrame.cols, ioFrame.rows);
    if (roi.area() <= 0) return;

    // The shape is rasterized into a mask so only the rounded area is mixed, and so the
    // corners stay smooth where the panel meets the frame
    cv::Mat mask(roi.size(), CV_8UC1, cv::Scalar(0));
    fill_rounded_rect(mask, cv::Rect(iRect.x - roi.x, iRect.y - roi.y, iRect.width, iRect.height), cv::Scalar(255), iRadius);

    cv::Mat patch = ioFrame(roi);
    const cv::Mat tint(roi.size(), patch.type(), iColor);

    cv::Mat blended;
    cv::addWeighted(patch, 1.0 - iAlpha, tint, iAlpha, 0.0, blended);
    blended.copyTo(patch, mask);
  }

  void add_glow(cv::Mat& ioFrame, const cv::Mat& iLayer, const cv::Rect& iRoi, double iStrength, int iBlurSize)
  {
    const cv::Rect roi = iRoi & cv::Rect(0, 0, ioFrame.cols, ioFrame.rows);
    if (roi.area() <= 0 || iLayer.size() != ioFrame.size()) return;

    const int blur = (iBlurSize | 1);

    cv::Mat halo;
    cv::GaussianBlur(iLayer(roi), halo, { blur, blur }, 0.0);

    cv::Mat patch = ioFrame(roi);
    cv::addWeighted(patch, 1.0, halo, iStrength, 0.0, patch);
  }

  void draw_contrast_polyline(cv::Mat& ioFrame, const std::vector<cv::Point>& iPoints, bool iClosed, const cv::Scalar& iColor, int iThickness)
  {
    if (iPoints.size() < 2U) return;

    cv::polylines(ioFrame, iPoints, iClosed, cv::Scalar::all(0.0), iThickness + 2, cv::LINE_AA);
    cv::polylines(ioFrame, iPoints, iClosed, iColor, iThickness, cv::LINE_AA);
  }
}
