#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Messaging/Message.h"

#include <opencv2/core/core.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The fitted feature points of one tracked face. The rectangle is the bounding
  /// box of the shape, which frames the face tighter than the tracker's rectangle.
  struct ShapeDescriptor
  {
    int trackId = 0;
    cv::Rect faceRect;
    fw::VectorPt2D shape2D;
  };

  /// @brief The shapes fitted on one frame, keyed by track. Carries the frame size as well:
  /// the shapes are coordinates in that frame, and consumers such as the pose estimation
  /// derive the camera matrix from it without needing the image itself.
  class ShapeMessage : public fw::Message
  {
  public:

    using ShapeVector = std::vector<ShapeDescriptor>;

    ShapeMessage(ShapeVector iShapes, const cv::Size& iFrameSize, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mShapes(std::move(iShapes)),
      mFrameSize(iFrameSize)
    {
    }

    virtual ~ShapeMessage() = default;

    inline bool IsEmpty() const
    {
      return mShapes.empty();
    }

    inline std::size_t GetSize() const
    {
      return mShapes.size();
    }

    inline const ShapeVector& GetShapes() const
    {
      return mShapes;
    }

    inline const cv::Size& GetFrameSize() const
    {
      return mFrameSize;
    }

  private:
    ShapeVector mShapes;
    cv::Size mFrameSize;
  };
}
