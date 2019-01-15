#pragma once

#include <vector>
#include <memory>
#include <opencv2/core/core.hpp>

#include <clm/CLM.h>
#include <clm/FCheck.h>

#include "Framework/Util.h"

namespace face
{
	class ShapeModel
	{
	public:
		typedef std::shared_ptr<ShapeModel> Shared;

		ShapeModel();

		~ShapeModel() = default;

		void InitShape(const cv::Rect& iRect);

		void ShiftShape(const cv::Point& iOffset);

		void Fit(const cv::Mat& iFrame, std::vector<int> &iWinSize, int iNoIter, double iClamp, double iFTol);

		bool FailureCheck(const cv::Mat& iFrame);

		bool GetMinMax2D(const cv::Rect& iRect, cv::Point2d& oMin, cv::Point2d& oMax) const;

		inline const cv::Mat& GetShape2D() const
		{
			return mShape2D;
		}

		inline const cv::Mat& GetRefShape() const
		{
			return mRefShape;
		}

		inline const cv::Scalar& GetSimilarity() const
		{
			return mSimilarity;
		}

	private:
		int mNumberOfPts = 0;
		FACETRACKER::CLM mCLM;					///< Constrained Local Model
		FACETRACKER::MFCheck mFailureCheck;		///< Checks for Tracking Failure
		cv::Mat mShape2D;						///< Current 2D shape
		cv::Mat mRefShape;						///< Reference shape model
		cv::Scalar mSimilarity;					///< Initialization similarity
	};
}
