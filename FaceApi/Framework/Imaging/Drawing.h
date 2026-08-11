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
