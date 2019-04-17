#pragma once

#include "Framework/Util.h"
#include "Framework/UtilOCV.h"

#include <clm/CLM.h>
#include <clm/FCheck.h>
#include <opencv2/core/core.hpp>
#include <vector>

namespace face
{
	class ClmWrapper
	{
	public:
		static ClmWrapper& GetInstance();

		fw::ErrorCode Initialize(const std::string& iTrackerFile, const std::string& iTriFile, const std::string& iConFile);

		inline bool IsInitialized() const
		{
			return mInitialized;
		}

		inline int GetCount() const
		{
			return mCount;
		}

		inline const FACETRACKER::CLM& GetCLM() const
		{
			return mCLM;
		}

		inline const FACETRACKER::MFCheck& GetFailureCheck() const
		{
			return mFailureCheck;
		}

		inline const cv::Mat& GetReferenceShapeMat2D() const
		{
			return mReferenceShapeMat2D;
		}

		inline const fw::ocv::VectorPt2D& GetReferenceShape2D() const
		{
			return mReferenceShape2D;
		}

		inline const fw::ocv::VectorPt3D& GetReferenceShape3D() const
		{
			return mReferenceShape3D;
		}

		inline const cv::Scalar& GetSimil() const
		{
			return mSimil;
		}

	private:
		fw::ErrorCode Load(const std::string& iTrackerFile, const std::string& iTriFile, const std::string& iConFile);

		FACETRACKER::CLM mCLM;				///< The CLM computing model
		FACETRACKER::MFCheck mFailureCheck; ///< Failure checker

		cv::Mat mReferenceShapeMat2D;		///< The 2D reference shape points
		fw::ocv::VectorPt2D mReferenceShape2D;	///< The 2D reference shape points
		fw::ocv::VectorPt3D mReferenceShape3D;	///< The 3D reference shape points
		cv::Scalar mSimil;					///< The similarity map

		bool mInitialized = false;			///< true if the files are loaded properly
		int mCount = 0;						///< The number of the shape points
	};
}
