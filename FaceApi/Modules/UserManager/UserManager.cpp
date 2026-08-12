#include "Modules/UserManager/UserManager.h"

namespace face
{
  std::shared_ptr<UserSnapshotMessage> UserManager::Main(std::shared_ptr<FaceDataMessage> iFaces)
  {
    DrainCommands();

    if (!iFaces || iFaces->IsEmpty()) return nullptr;

    UserSnapshotMessage::UserVector users;
    users.reserve(iFaces->GetSize());

    for (const auto& entry : iFaces->GetEntries())
      users.emplace_back(std::make_shared<const User>(entry.track, entry.data));

    return std::make_shared<UserSnapshotMessage>(std::move(users), iFaces->GetFrameId(), iFaces->GetTimestamp());
  }
}
