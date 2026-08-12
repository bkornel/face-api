#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Messaging/Message.h"

#include <opencv2/core/core.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The head pose of one tracked face: the 6DoF pose to the camera, the rotated 3-D
  /// feature points and the projectable face box, plus what re-projecting them needs.
  struct PoseDescriptor
  {
    int trackId = 0;

    cv::Vec3d rpy;
    cv::Vec3d position3D;

    cv::Mat cameraMatrix;
    cv::Mat extrinsics;
    cv::Mat rvec;
    cv::Mat tvec;

    fw::VectorPt3D shape3D;
    fw::VectorPt3D faceBox;
  };

  /// @brief The head poses estimated on one frame, keyed by track
  class PoseMessage : public fw::Message
  {
  public:

    using PoseVector = std::vector<PoseDescriptor>;

    PoseMessage(PoseVector iPoses, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mPoses(std::move(iPoses))
    {
    }

    virtual ~PoseMessage() = default;

    inline bool IsEmpty() const
    {
      return mPoses.empty();
    }

    inline std::size_t GetSize() const
    {
      return mPoses.size();
    }

    inline const PoseVector& GetPoses() const
    {
      return mPoses;
    }

  private:
    PoseVector mPoses;
  };
}
