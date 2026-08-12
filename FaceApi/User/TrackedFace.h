#pragma once

#include "Framework/TimeExtensions.h"

#include <opencv2/core/core.hpp>

namespace face
{
  enum class TrackStatus
  {
    Detected = 0,
    Tracked,
    Inactive
  };

  /// @brief One face the tracker follows, as plain data: where it is, who it is and since
  /// when. The tracker owns the identity; everything downstream only reads it.
  struct TrackedFace
  {
    int trackId = 0;
    TrackStatus status = TrackStatus::Detected;

    cv::Rect faceRect;

    fw::Timestamp creationTs;
    fw::Timestamp lastDetectionTs;
    fw::Timestamp lastUpdateTs;
  };
}
