#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/Messaging/Message.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace face
{
  /// @brief The reference-aligned shapes of one tracked face. The 3-D half needs the head
  /// pose; when the pose is not part of the graph it stays empty.
  struct NormShapeDescriptor
  {
    int trackId = 0;
    fw::VectorPt2D normShape2D;
    fw::VectorPt3D normShape3D;
  };

  /// @brief The normalized shapes of one frame, keyed by track
  class NormShapeMessage : public fw::Message
  {
  public:

    using NormShapeVector = std::vector<NormShapeDescriptor>;

    NormShapeMessage(NormShapeVector iShapes, uint32_t iFrameId, fw::Timestamp iTimestamp) :
      Message(iFrameId, iTimestamp),
      mShapes(std::move(iShapes))
    {
    }

    virtual ~NormShapeMessage() = default;

    inline bool IsEmpty() const
    {
      return mShapes.empty();
    }

    inline std::size_t GetSize() const
    {
      return mShapes.size();
    }

    inline const NormShapeVector& GetShapes() const
    {
      return mShapes;
    }

  private:
    NormShapeVector mShapes;
  };
}
