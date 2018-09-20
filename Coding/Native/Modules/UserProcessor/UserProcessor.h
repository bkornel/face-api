#pragma once

#include "Common/Configuration.h"
#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

#include "User/User.h"
#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"
#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.h"

namespace face
{
	class UserProcessor :
		public fw::Module,
		public fw::Port<ActiveUsersMessage::Shared>
	{
	public:
		UserProcessor();

		virtual ~UserProcessor() = default;

		ActiveUsersMessage::Shared Process(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iActiveUsers);

	private:
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		ShapeModelDispatcher mShapeModelDispatcher;
		PoseEstimationDispatcher mPoseEstimationDispatcher;
		ShapeNormDispatcher mShapeNormDispatcher;
	};
}
