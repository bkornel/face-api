#pragma once

#include "FaceResult.h"

namespace face
{
  /// @brief Flat float serialization of FaceResults, for hosts that cannot take C++ structs
  /// across their language boundary - a JNI float[], a C callback, a shared memory block.
  ///
  /// The buffer is meant to be allocated once and reused, so nothing is allocated per frame.
  /// Layout, all values in the pixel coordinate system of the processed frame:
  ///
  ///   [0]                    number of faces written
  ///   then, per face, cFaceStride floats:
  ///     [0]                  user id
  ///     [1]                  frame id
  ///     [2..5]               face rectangle, x y width height
  ///     [6]                  number of landmarks that follow
  ///     [7]                  number of face box points that follow
  ///     [8..]                landmarks, x y each, cMaxLandmarks slots
  ///     [..]                 face box projected to 2-D, x y each, cMaxBoxPoints slots
  ///     [..]                 roll pitch yaw, radians
  ///     [..]                 position in the 3-D camera coordinate system
  namespace result_buffer
  {
    constexpr int cHeaderFloats = 1;
    constexpr int cMaxLandmarks = 66;
    constexpr int cMaxBoxPoints = 8;
    constexpr int cFaceStride = 8 + (cMaxLandmarks * 2) + (cMaxBoxPoints * 2) + 3 + 3;

    constexpr int cOffsetUserId = 0;
    constexpr int cOffsetFrameId = 1;
    constexpr int cOffsetRect = 2;
    constexpr int cOffsetLandmarkCount = 6;
    constexpr int cOffsetBoxCount = 7;
    constexpr int cOffsetLandmarks = 8;
    constexpr int cOffsetBox = cOffsetLandmarks + (cMaxLandmarks * 2);
    constexpr int cOffsetPose = cOffsetBox + (cMaxBoxPoints * 2);

    /// @brief Number of floats needed to carry iMaxFaces faces
    int required_size(int iMaxFaces);

    /// @brief Writes iResults into oBuffer, projecting the face box to 2-D on the way.
    /// Faces that do not fit in iCapacity are dropped.
    /// @return the number of faces written, or -1 if the buffer cannot hold even one
    int pack(const FaceResults& iResults, float* oBuffer, int iCapacity);
  }
}
