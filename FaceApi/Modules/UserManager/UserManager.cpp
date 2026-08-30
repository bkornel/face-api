#include "Modules/UserManager/UserManager.h"

#include <map>

namespace face
{
  namespace
  {
    // Takes a pointer on purpose: a reference and a ?: fallback would materialize a
    // temporary copy, and the map would point into it after it is gone
    template <typename DescriptorT>
    std::map<int, const DescriptorT*> ById(const std::vector<DescriptorT>* iDescriptors)
    {
      std::map<int, const DescriptorT*> byId;

      if (iDescriptors)
      {
        for (const auto& descriptor : *iDescriptors)
          byId.emplace(descriptor.trackId, &descriptor);
      }

      return byId;
    }
  }

  std::shared_ptr<UserSnapshotMessage> UserManager::Main(std::shared_ptr<FaceTrackMessage> iTracks,
                                                         std::shared_ptr<ShapeMessage> iShapes,
                                                         std::shared_ptr<PoseMessage> iPoses,
                                                         std::shared_ptr<NormShapeMessage> iNormShapes)
  {
    DrainCommands();

    if (!iTracks || iTracks->IsEmpty()) return nullptr;

    const auto shapeById = ById(iShapes ? &iShapes->GetShapes() : nullptr);
    const auto poseById = ById(iPoses ? &iPoses->GetPoses() : nullptr);
    const auto normById = ById(iNormShapes ? &iNormShapes->GetShapes() : nullptr);

    UserSnapshotMessage::UserVector users;
    users.reserve(iTracks->GetSize());

    for (const auto& track : iTracks->GetTracks())
    {
      TrackedFace face = track;
      UserData data;

      data.SetFaceRect(track.faceRect);

      if (auto it = shapeById.find(track.trackId); it != shapeById.end())
      {
        data.SetShape2D(it->second->shape2D);

        // A pure function of the shape this record already holds, so it is read here rather
        // than by a module of its own: there is nothing to schedule and nothing to share
        data.SetExpression(measure_expression(it->second->shape2D));

        // The shape's bounding box frames the face tighter than the tracker's rectangle
        data.SetFaceRect(it->second->faceRect);
        face.faceRect = it->second->faceRect;
      }

      if (auto it = poseById.find(track.trackId); it != poseById.end())
      {
        const PoseDescriptor& pose = *it->second;

        data.SetPose(pose.rpy, pose.position3D);
        data.SetCameraMatrix(pose.cameraMatrix);
        data.SetExtrinsics(pose.extrinsics, pose.rvec, pose.tvec);
        data.SetShape3D(pose.shape3D);
        data.SetFaceBox(pose.faceBox);
      }

      if (auto it = normById.find(track.trackId); it != normById.end())
      {
        data.SetNormShapes(it->second->normShape2D, it->second->normShape3D);
      }

      users.emplace_back(std::make_shared<const User>(face, data));
    }

    return std::make_shared<UserSnapshotMessage>(std::move(users), iTracks->GetFrameId(), iTracks->GetTimestamp());
  }
}
