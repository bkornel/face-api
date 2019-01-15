#pragma once

#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "Util.h"

namespace fw
{
	namespace ocv
	{
		typedef std::vector<cv::Point2d> VectorPt2D;
		typedef std::vector<cv::Point3d> VectorPt3D;

		struct Font
		{
			int fontface = cv::FONT_HERSHEY_SIMPLEX;
			double scale = 0.35;
			int thickness = 1;
			int lineType = cv::LINE_AA;
		};

		template<typename _Tp>
		inline bool equals(const cv::Rect_<_Tp>& a, const cv::Rect_<_Tp>& b)
		{
			return fw::equals(a.x, b.x) && fw::equals(a.y, b.y) && fw::equals(a.width, b.width) && fw::equals(a.height, b.height);
		}

		template<typename _Tp>
		inline cv::Rect_<_Tp> scale_rect(const cv::Rect_<_Tp>& rect, float scale)
		{
			cv::Rect_<float> fr = rect;

			fr += cv::Point2f((1.0F - scale) * rect.width / 2.0F, (1.0F - scale) * rect.height / 2.0F);
			fr.width *= scale;
			fr.height *= scale;

			cv::Rect_<_Tp> rr((_Tp)fr.x, (_Tp)fr.y, (_Tp)fr.width, (_Tp)fr.height);

			return rr.area() <= 0 ? rect : rr;
		}

		template<typename _Tp>
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

		template<typename _Tp>
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

		float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2);

		void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation);

		void put_text(const std::string& iText, const cv::Point& iLocation, cv::Mat& oFrame, bool iUseBG = false, const Font& iFont = Font());

		cv::Scalar get_color(double iV, double iMin, double iMax);

		void project_point(const cv::Point3d& iPoint3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, cv::Point2d& oPoint2D);

		void project_point(const VectorPt3D& iPoints3D, const cv::Mat& iRvec, const cv::Mat& iTvec, const cv::Mat& iCameraMatrix, VectorPt2D& oPoints2D);

		cv::Mat get_camera_matrix(const cv::Size& iSize);

		double sum_squared(const cv::Mat& iMatrix);

		bool get_value(const cv::FileNode& iNode, const std::string& iName, std::string& oValue);
	}
}
