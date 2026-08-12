#pragma once

#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/ActiveUsersMessage.h"
#include "Messages/UserSnapshotMessage.h"

#include <memory>

namespace face
{
  /// @brief The boundary between the tracking and its readers: turns the live users into an
  /// immutable snapshot. Everything that leaves the processing chain leaves through here.
  class UserSnapshot : public fw::Module,
                       public fw::Port<std::shared_ptr<UserSnapshotMessage>(std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    UserSnapshot() = default;

    virtual ~UserSnapshot() = default;

    std::shared_ptr<UserSnapshotMessage> Main(std::shared_ptr<ActiveUsersMessage> iUsers) override;
  };
}
