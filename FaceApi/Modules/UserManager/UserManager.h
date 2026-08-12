#pragma once

#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include "Messages/FaceTrackMessage.h"
#include "Messages/NormShapeMessage.h"
#include "Messages/PoseMessage.h"
#include "Messages/ShapeMessage.h"
#include "Messages/UserSnapshotMessage.h"

#include <memory>

namespace face
{
  /// @brief Composes the final users: one record per track, joined by track id with whatever
  /// the estimator modules produced for it. Only the tracks are required - the other inputs
  /// are optional ports, so a graph that skips the pose or the normalization simply leaves
  /// those fields empty. It is the only place a User is ever built.
  class UserManager : public fw::Module,
                      public fw::Port<std::shared_ptr<UserSnapshotMessage>(std::shared_ptr<FaceTrackMessage>,
                                                                           std::shared_ptr<ShapeMessage>,
                                                                           std::shared_ptr<PoseMessage>,
                                                                           std::shared_ptr<NormShapeMessage>)>
  {
  public:

    UserManager() = default;

    virtual ~UserManager() = default;

    std::shared_ptr<UserSnapshotMessage> Main(std::shared_ptr<FaceTrackMessage> iTracks,
                                              std::shared_ptr<ShapeMessage> iShapes,
                                              std::shared_ptr<PoseMessage> iPoses,
                                              std::shared_ptr<NormShapeMessage> iNormShapes) override;
  };
}
