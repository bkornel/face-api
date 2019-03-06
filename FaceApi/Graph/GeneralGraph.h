#pragma once

#include <string>
#include <vector>

#include "Graph/Graph.h"

namespace face
{
	class ImageQueue;
	class FaceDetection;
	class UserManager;
	class UserProcessor;
	class UserHistory;
	class Visualizer;

	class GeneralGraph :
		public Graph
	{
	public:
		GeneralGraph();

		GeneralGraph(const GeneralGraph& iOther) = delete;

		virtual ~GeneralGraph();

		GeneralGraph& operator=(const GeneralGraph& iOther) = delete;

	protected:
		fw::ErrorCode Connect() override;

		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

		fw::ErrorCode DeInitializeInternal() override;

	private:
		ImageQueue* mImageQueue = nullptr;
		FaceDetection* mFaceDetection = nullptr;
		UserManager* mUserManager = nullptr;
		UserProcessor* mUserProcessor = nullptr;
		UserHistory* mUserHistory = nullptr;
		Visualizer* mVisualizer = nullptr;
	};
}
