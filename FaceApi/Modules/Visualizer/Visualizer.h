#pragma once

#include <vector>
#include <opencv2/core/core.hpp>

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"
#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

namespace face
{
	class Visualizer :
		public fw::Module,
		public fw::Port<bool(ImageMessage::Shared, ActiveUsersMessage::Shared)>
	{
	public:
		Visualizer();

		virtual ~Visualizer() = default;

		bool Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iUsers) override;

		inline bool HasOutput() const
		{
			return !mResultImage.empty();
		}

		inline const cv::Mat& GetResultImage() const
		{
			return mResultImage;
		}

		inline void SetQueueSize(int iQueueSize)
		{
			mQueueSize = iQueueSize;
		}

	private:
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		void DrawShapeModel(const User& iUser, cv::Mat& oImage) const;

		void DrawUserData(const User& iUser, cv::Mat& oImage) const;

		void DrawAxes(const User& iUser, cv::Mat& oImage) const;

		void DrawBoundingBox(const User& iUser, cv::Mat& oImage, int iSegmentWidth = 5, int iThickness = 1) const;

		void DrawGeneral(double iRuntime, int iFrameID, cv::Mat& oImage) const;

		void CreateShapeColorMap(const fw::ocv::VectorPt3D& iShape3D, cv::Mat& oColorMap) const;

		cv::Mat mResultImage;
		std::vector<cv::Scalar> mColorsOfAxes;
		int mQueueSize = 0;
	};
}
