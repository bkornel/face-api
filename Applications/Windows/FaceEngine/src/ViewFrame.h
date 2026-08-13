#pragma once

#include "FaceResult.h"

#include <opencv2/core/core.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace fe
{
  /// @brief A completed frame and everything the API determined about it.
  ///
  /// The faces are the API's results unchanged - it reports the identity's state, the
  /// normalised shapes and the expression measures itself, so there is nothing to derive
  /// on the way past. What this adds is what only this application knows: the image, how
  /// long the frame took, and whether the pipeline already drew on it.
  ///
  /// Handed around by shared pointer, so a renderer that is still drawing one frame cannot
  /// have it replaced underneath by the next.
  struct ViewFrame
  {
    /// @brief The frame as it came out of the pipeline, BGR
    cv::Mat image;

    uint32_t frameId = 0U;

    /// @brief Capture time, epoch milliseconds
    int64_t timestampMs = 0LL;

    /// @brief Milliseconds from capture to completion
    double latencyMs = 0.0;

    /// @brief True when the pipeline's own Visualizer already drew on the image, in which
    /// case it is presented as it is instead of a second overlay going over the first
    bool imageHasOverlay = false;

    std::vector<face::FaceResult> faces;
  };

  using ViewFramePtr = std::shared_ptr<const ViewFrame>;
}
