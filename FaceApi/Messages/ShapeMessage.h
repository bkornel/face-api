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

    /// @brief The same landmarks in three dimensions, in the model's own space: the head
    /// pose is not in them, so what is left is whose face it is and what it is doing.
    ///
    /// The morphable model produces this and the 2-D shape is its projection, so it costs
    /// nothing to report - and it is the only place in the pipeline where the shape of a
    /// face exists in three dimensions. What the pose module produces is the canonical model
    /// moved to where the head is, which carries no expression at all.
    ///
    /// Same units and orientation as FaceModel's canonical shape: millimetres, x right,
    /// y down, z away, the nose tip at the origin.
    fw::VectorPt3D shape3D;

    /// @brief The head rotation the fitter regressed, in the same frame as shape3D.
    ///
    /// The morphable model does not only reconstruct a face, it also reports where that face
    /// is pointing - the pose is what turns its 3-D landmarks into the 2-D ones. It is worth
    /// carrying because it is regressed rather than solved: it cannot land on the mirrored
    /// twin that a pose recovered from correspondences alone always admits.
    ///
    /// Identity when the fitter did not report one, which iHasRotation distinguishes.
    cv::Matx33d rotation = cv::Matx33d::eye();

    bool hasRotation = false;
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
