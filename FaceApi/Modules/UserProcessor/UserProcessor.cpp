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

        // These live under the "shapeModel" node, not under the module node.
        if (fw::ocv::get_value(shapeModelNode, "trackerFile", value))
          trackerFile = value;

        if (fw::ocv::get_value(shapeModelNode, "triFile", value))
          triFile = value;

        if (fw::ocv::get_value(shapeModelNode, "conFile", value))
          conFile = value;

        mShapeModelDispatcher.Initialize(shapeModelNode);
      }

      mPoseEstimationDispatcher.Initialize(iSettings["headPose"]);
      mShapeNormDispatcher.Initialize(iSettings["shapeNorm"]);
    }

    return ClmWrapper::GetInstance().Initialize(trackerFile, triFile, conFile);
  }

  std::shared_ptr<ActiveUsersMessage> UserProcessor::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if ((!iImage || iImage->IsEmpty()) || (!iUsers || iUsers->IsEmpty()))
      return nullptr;

    FACE_PROFILER(2_User_Processor);

    mShapeModelDispatcher.BeginFrame(iImage->GetFrameGray());

    const auto& activeUsers = iUsers->GetActiveUsers();
    for (const auto& user : activeUsers)
    {
      if (user->AcceptDispatcher(mShapeModelDispatcher))
      {
        mPoseEstimationDispatcher.EstimateCameraMatrix(iImage->GetSize());

        user->AcceptDispatcher(mPoseEstimationDispatcher);
        user->AcceptDispatcher(mShapeNormDispatcher);
      }
    }

    mShapeModelDispatcher.EndFrame();

    // The users above are the live ones UserManager keeps tracking, and writing the
    // refined face rect back into them is what feeds the next frame's tracking. What
    // leaves the module is a snapshot though, so nothing downstream can observe a user
    // being updated, nor is anyone else's view of it changed from here.
    ActiveUsersMessage::UserVector snapshot;
    for (const auto& user : activeUsers)
    {
      if (user && user->IsActive())
        snapshot.emplace_back(std::make_shared<User>(*user));
    }

    if (snapshot.empty()) return nullptr;

    return std::make_shared<ActiveUsersMessage>(snapshot, iUsers->GetFrameId(), iUsers->GetTimestamp());
  }
}
