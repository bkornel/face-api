#pragma once

#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceDataMessage.h"
#include "Messages/UserSnapshotMessage.h"

#include <memory>

namespace face
{
  /// @brief The end of the processing chain: composes the raw per-face results into
  /// immutable User records. It is the only place a User is ever built, so what a user
  /// consists of is decided here and nowhere else.
  class UserManager : public fw::Module,
                      public fw::Port<std::shared_ptr<UserSnapshotMessage>(std::shared_ptr<FaceDataMessage>)>
  {
  public:

    UserManager() = default;

    virtual ~UserManager() = default;

    std::shared_ptr<UserSnapshotMessage> Main(std::shared_ptr<FaceDataMessage> iFaces) override;
  };
}
