#include "Graph/GeneralGraph.h"

#include "Framework/Functional.hpp"

#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"

namespace face
{
	GeneralGraph::GeneralGraph() :
		Graph("GeneralGraph")
	{
	}

	GeneralGraph::~GeneralGraph()
	{
	}

	fw::ErrorCode GeneralGraph::Connect()
	{
		// This the first node in the execution
		mFirstNode.port = fw::connect(
			FW_BIND(&fw::FirstNode<unsigned>::Main, &mFirstNode),
			mExecutor);

		// Image queue: popping out a frame
		mImageQueue->port = fw::connect(
			FW_BIND(&ImageQueue::Pop, mImageQueue),
			mFirstNode.port.second);

		// Face detector: detect faces on the frame pop from the queue
		mFaceDetection->port = fw::connect(
			FW_BIND(&FaceDetection::Detect, mFaceDetection),
			mImageQueue->port);

		// User manager: manage active and inactive users, update them with the detected faces
		mUserManager->port = fw::connect(
			FW_BIND(&UserManager::Process, mUserManager),
			mImageQueue->port, mFaceDetection->port);

		// User processor: extract all of the features from faces (e.g. shape model, head pose)
		mUserProcessor->port = fw::connect(
			FW_BIND(&UserProcessor::Process, mUserProcessor),
			mImageQueue->port, mUserManager->port);

		// User history: maintain all entries that have been estimated by the user processor module in time
		mUserHistory->port = fw::connect(
			FW_BIND(&UserHistory::Process, mUserHistory),
			mUserProcessor->port);

		// Visualizer: generate and draw the results
		mVisualizer->port = fw::connect(
			FW_BIND(&Visualizer::Draw, mVisualizer),
			mImageQueue->port, mUserProcessor->port);

		// Last node: waiting for the results
		mLastNode.port = fw::connect(
			FW_BIND(&fw::LastNode<bool>::Main, &mLastNode),
			mVisualizer->port);

		return fw::ErrorCode::OK;
	}
}
