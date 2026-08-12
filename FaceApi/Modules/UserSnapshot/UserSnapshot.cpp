#include "Modules/UserSnapshot/UserSnapshot.h"

namespace face
{
  std::shared_ptr<UserSnapshotMessage> UserSnapshot::Main(std::shared_ptr<ActiveUsersMessage> iUsers)
  {
    DrainCommands();

    if (!iUsers || iUsers->IsEmpty()) return nullptr;

    // The message constructor does the copying, so nothing downstream can reach the
    // users the tracking keeps writing to
    auto snapshot = std::make_shared<UserSnapshotMessage>(iUsers->GetActiveUsers(), iUsers->GetFrameId(), iUsers->GetTimestamp());

    return snapshot->IsEmpty() ? nullptr : snapshot;
  }
}
