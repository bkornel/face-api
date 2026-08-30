#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <string>

namespace fw
{
  struct Font
  {
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.35;
    int thickness = 1;
    int lineType = cv::LINE_AA;
  };

  void put_text(const std::string& iText, const cv::Point& iLocation, cv::Mat& oFrame, bool iUseBG = false, const Font& iFont = Font());

  // Maps iV onto a blue-to-red ramp, clamped to the [iMin, iMax] interval
  cv::Scalar get_color(double iV, double iMin, double iMax);

  /// @brief Fills a rectangle with rounded corners, clipped to the frame
  void fill_rounded_rect(cv::Mat& ioFrame, const cv::Rect& iRect, const cv::Scalar& iColor, int iRadius);

  /// @brief Mixes iColor into the frame inside a rounded rectangle, iAlpha of it at full
  /// strength. What panels are made of: readable over any frame without hiding it.
  void blend_rounded_rect(cv::Mat& ioFrame, const cv::Rect& iRect, const cv::Scalar& iColor, int iRadius, double iAlpha);

  /// @brief Adds a blurred copy of iLayer onto the frame, which turns whatever was drawn
  /// into iLayer into a halo around itself. Restricted to iRoi, so the cost follows the
  /// area that was actually drawn on rather than the frame size.
  void add_glow(cv::Mat& ioFrame, const cv::Mat& iLayer, const cv::Rect& iRoi, double iStrength, int iBlurSize);

  /// @brief A line with a dark pass underneath, so it reads on a light and a dark frame alike
  void draw_contrast_polyline(cv::Mat& ioFrame, const std::vector<cv::Point>& iPoints, bool iClosed, const cv::Scalar& iColor, int iThickness);

  template <typename _Tp>
  void draw_dotted_line(cv::Mat& ioFrame, const cv::Point_<_Tp>& iPt1, const cv::Point_<_Tp>& iPt2, const cv::Scalar& iColor, int iSegmentWidth = 5, int iThickness = 1)
  {
    cv::LineIterator it(ioFrame, iPt1, iPt2);
    if (it.count <= 0)
      return;

    cv::Point_<_Tp> pt1 = it.pos(), pt2 = it.pos();
    int segmentCount = 0;

    for (int i = 0; i < it.count; i++, it++)
    {
      if (i % iSegmentWidth == 0)
      {
        pt1 = it.pos();

        if (++segmentCount % 2 == 1)
          cv::line(ioFrame, pt1, pt2, iColor, iThickness, cv::LINE_AA);

        pt2 = pt1;
      }
    }

    cv::line(ioFrame, pt1, pt2, iColor, iThickness, cv::LINE_AA);
  }

  template <typename _Tp>
  void draw_dotted_rect(cv::Mat& ioFrame, const cv::Rect_<_Tp>& iRect, const cv::Scalar& iColor, int iSegmentWidth = 5, int iThickness = 1)
  {
    const cv::Point_<_Tp> tl = iRect.tl();
    const cv::Point_<_Tp> br = iRect.br();
    const cv::Point_<_Tp> tr(tl.x + iRect.width, tl.y);
    const cv::Point_<_Tp> bl(br.x - iRect.width, br.y);

    draw_dotted_line(ioFrame, tl, tr, iColor, iSegmentWidth, iThickness);
    draw_dotted_line(ioFrame, tr, br, iColor, iSegmentWidth, iThickness);
    draw_dotted_line(ioFrame, br, bl, iColor, iSegmentWidth, iThickness);
    draw_dotted_line(ioFrame, bl, tl, iColor, iSegmentWidth, iThickness);
  }
}
