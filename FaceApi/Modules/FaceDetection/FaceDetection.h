#pragma once

#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include "Common/Configuration.h"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"
#include "Framework/Stopwatch.h"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"

namespace face
{
	class FaceDetection :
		public fw::Module,
		public fw::Port<RoiMessage::Shared(ImageMessage::Shared)>
	{
	public:
		FaceDetection();

		virtual ~FaceDetection() = default;

		RoiMessage::Shared Main(ImageMessage::Shared iImage) override;

		inline float GetMinSize() const
		{
			return mMinSize;
		}

		inline void SetForceRun(bool iForceRun)
		{
			mForceRun |= iForceRun;
		}

	protected:
		const static float sForceDetectionSec;

		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		void RemoveMultipleDetections(std::vector<cv::Rect>& ioDetections);

		bool RunDetectection() const;

		cv::CascadeClassifier mCascadeClassifier;   ///< The OpenCV cascade classifier
		fw::Stopwatch mDetectionSW;

		// General parameters
		std::string mCascadeFile = "haarcascade_frontalface_alt2.xml";
		float mImageScaleFactor = 1.0F;
		float mImageScaleFactorInv = 1.0F;
		float mDetectionOverlap = 0.2F;
		float mDetectionSec = 10.0F;
		bool mForceRun = false;

		// Parameter of detectMultiScale(...)
		float mScaleFactor = 1.1F;
		int mMinNeighbors = 3;
		float mMinSize = 0.05f;
		float mMaxSize = 1.0F;
		int mFlags = 0;
	};
}
