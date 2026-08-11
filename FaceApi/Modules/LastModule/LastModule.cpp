#include "Framework/ErrorCode.h"
#include "Modules/LastModule/LastModule.h"

#include "User/User.h"

namespace face
{
  bool LastModule::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    const bool hasImage = (iImage != nullptr && !iImage->IsEmpty());

    // Only a real frame is remembered. The graph ticks far more often than frames arrive,
    // so overwriting this with the empty ticks in between left GetLastFrameId() reading 0
    // almost every time it was asked. Process() gates on HasOutput(), which is this
    // return value, so holding the previous frame here cannot republish a stale one.
    if (hasImage)
    {
      // Read from the app thread through FaceApi::GetLastFrameId() and friends
      std::lock_guard<std::mutex> lock(mLastMutex);
      mLastImage = iImage;
      mLastUsers = iUsers;
    }

    return hasImage;
  }

  void LastModule::Clear()
  {
    std::lock_guard<std::mutex> lock(mLastMutex);
    mLastImage = nullptr;
    mLastUsers = nullptr;
  }

  std::shared_ptr<ImageMessage> LastModule::GetLastImage() const
  {
    std::lock_guard<std::mutex> lock(mLastMutex);
    return mLastImage;
  }

  unsigned LastModule::GetLastFrameId() const
  {
    std::shared_ptr<ImageMessage> lastImage = GetLastImage();
    return lastImage ? lastImage->GetFrameId() : 0U;
  }

  long long LastModule::GetLastTimestamp() const
  {
    std::shared_ptr<ImageMessage> lastImage = GetLastImage();
    return lastImage ? fw::to_epoch_ms(lastImage->GetTimestamp()) : 0LL;
  }

  fw::ErrorCode LastModule::GetLastResults(FaceResults& oResults) const
  {
    oResults.clear();

    std::shared_ptr<ActiveUsersMessage> users;
    {
      std::lock_guard<std::mutex> lock(mLastMutex);
      users = mLastUsers;
    }

    // The input port carrying the users is optional, so it may simply not be connected
    if (!users || users->IsEmpty()) return fw::ErrorCode::NotFound;

    const auto& activeUsers = users->GetActiveUsers();
    oResults.reserve(activeUsers.size());

    for (const auto& user : activeUsers)
    {
      if (!user) continue;

      FaceResult result;
      result.userId = user->GetUserId();
      result.frameId = users->GetFrameId();
      result.timestamp = fw::to_epoch_ms(users->GetTimestamp());
      result.faceRect = user->GetFaceRect();
      result.shape2D = user->GetShape2D();
      result.shape3D = user->GetShape3D();
      result.faceBox = user->GetFaceBox();
      result.rpy = user->GetRPY();
      result.position3D = user->GetPosition3D();
      result.cameraMatrix = user->GetCameraMatrix();
      result.rvec = user->GetRvec();
      result.tvec = user->GetTvec();

      oResults.emplace_back(std::move(result));
    }

    return oResults.empty() ? fw::ErrorCode::NotFound : fw::ErrorCode::OK;
  }
}
