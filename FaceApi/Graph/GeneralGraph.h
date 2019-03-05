#include <string>
#include <vector>

#include "Graph/Graph.h"

namespace face
{
	class ImageQueue;
	class FaceDetection;
	class UserHistory;
	class UserManager;
	class UserProcessor;
	class Visualizer;

	class GeneralGraph :
		public Graph
	{
	public:
		GeneralGraph();

		virtual ~GeneralGraph();

	protected:
		fw::ErrorCode Connect() override;

	private:
		GeneralGraph(const GeneralGraph& iOther) = delete;

		GeneralGraph& operator=(const GeneralGraph& iOther) = delete;

		ImageQueue* mImageQueue = nullptr;
		FaceDetection* mFaceDetection = nullptr;
		UserHistory* mUserHistory = nullptr;
		UserManager* mUserManager = nullptr;
		UserProcessor* mUserProcessor = nullptr;
		Visualizer* mVisualizer = nullptr;
	};
}
