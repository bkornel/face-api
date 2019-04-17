#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/UserProcessor/ShapeModel/ClmWrapper.h"

#include "Common/Configuration.h"
#include "Framework/Profiler.h"

#include <easyloggingpp/easyloggingpp.h>
#include <iomanip>

namespace face
{
	fw::ErrorCode UserProcessor::InitializeInternal(const cv::FileNode& iSettings)
	{
		std::string trackerFile = "face.tracker";
		std::string triFile = "face.tri";
		std::string conFile = "face.con";

		if (!iSettings.empty())
		{
			const cv::FileNode shapeModelNode = iSettings["shapeModel"];

			if (!shapeModelNode.empty())
			{
				std::string value;

				if (fw::ocv::get_value(iSettings, "trackerFile", value))
					trackerFile = value;

				if (fw::ocv::get_value(iSettings, "triFile", value))
					triFile = value;

				if (fw::ocv::get_value(iSettings, "conFile", value))
					conFile = value;

				mShapeModelDispatcher.Initialize(shapeModelNode);
			}

			mPoseEstimationDispatcher.Initialize(iSettings["headPose"]);
			mShapeNormDispatcher.Initialize(iSettings["shapeNorm"]);
		}

		return ClmWrapper::GetInstance().Initialize(trackerFile, triFile, conFile);
	}

	ActiveUsersMessage::Shared UserProcessor::Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iUsers)
	{
		if ((!iImage || iImage->IsEmpty()) || (!iUsers || iUsers->IsEmpty()))
			return nullptr;

		FACE_PROFILER(2_User_Processor);

		mShapeModelDispatcher.SetFrame(iImage->GetFrameGray());
		
		const auto& activeUSers = iUsers->GetActiveUsers();
		for (const auto& user : activeUSers)
		{
			if (user->AcceptDispatcher(mShapeModelDispatcher))
			{
				mPoseEstimationDispatcher.EstimateCameraMatrix(iImage->GetSize());

				user->AcceptDispatcher(mPoseEstimationDispatcher);
				user->AcceptDispatcher(mShapeNormDispatcher);
			}
		}

		iUsers->RemoveInactiveUsers();

		return (iUsers->GetSize() > 0 ? iUsers : nullptr);
	}
}