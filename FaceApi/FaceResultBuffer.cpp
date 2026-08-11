#include "FaceResultBuffer.h"

#include "Framework/Ocv/Projection.h"

#include <algorithm>

namespace face
{
  namespace result_buffer
  {
    int required_size(int iMaxFaces)
    {
      return cHeaderFloats + ((std::max)(iMaxFaces, 1) * cFaceStride);
    }

    int pack(const FaceResults& iResults, float* oBuffer, int iCapacity)
    {
      if (!oBuffer || iCapacity < (cHeaderFloats + cFaceStride)) return -1;

      const int maxFaces = (iCapacity - cHeaderFloats) / cFaceStride;
      const int faceCount = (std::min)(static_cast<int>(iResults.size()), maxFaces);

      std::fill(oBuffer, oBuffer + cHeaderFloats + (faceCount * cFaceStride), 0.0F);
      oBuffer[0] = static_cast<float>(faceCount);

      for (int i = 0; i < faceCount; i++)
      {
        const FaceResult& result = iResults[static_cast<std::size_t>(i)];
        float* face = oBuffer + cHeaderFloats + (i * cFaceStride);

        face[cOffsetUserId] = static_cast<float>(result.userId);
        face[cOffsetFrameId] = static_cast<float>(result.frameId);
        face[cOffsetRect + 0] = static_cast<float>(result.faceRect.x);
        face[cOffsetRect + 1] = static_cast<float>(result.faceRect.y);
        face[cOffsetRect + 2] = static_cast<float>(result.faceRect.width);
        face[cOffsetRect + 3] = static_cast<float>(result.faceRect.height);

        const int landmarks = (std::min)(static_cast<int>(result.shape2D.size()), cMaxLandmarks);
        face[cOffsetLandmarkCount] = static_cast<float>(landmarks);

        for (int p = 0; p < landmarks; p++)
        {
          const cv::Point2d& pt = result.shape2D[static_cast<std::size_t>(p)];
          face[cOffsetLandmarks + (p * 2)] = static_cast<float>(pt.x);
          face[cOffsetLandmarks + (p * 2) + 1] = static_cast<float>(pt.y);
        }

        // Projected here rather than by the host: the projection needs the camera matrix and
        // the pose, and doing it once keeps every host from reimplementing it.
        int boxPoints = 0;

        if (!result.faceBox.empty() && !result.rvec.empty() && !result.tvec.empty() && !result.cameraMatrix.empty())
        {
          fw::ocv::VectorPt2D projected;
          fw::ocv::project_point(result.faceBox, result.rvec, result.tvec, result.cameraMatrix, projected);

          boxPoints = (std::min)(static_cast<int>(projected.size()), cMaxBoxPoints);
          for (int p = 0; p < boxPoints; p++)
          {
            face[cOffsetBox + (p * 2)] = static_cast<float>(projected[static_cast<std::size_t>(p)].x);
            face[cOffsetBox + (p * 2) + 1] = static_cast<float>(projected[static_cast<std::size_t>(p)].y);
          }
        }

        face[cOffsetBoxCount] = static_cast<float>(boxPoints);

        face[cOffsetPose + 0] = static_cast<float>(result.rpy[0]);
        face[cOffsetPose + 1] = static_cast<float>(result.rpy[1]);
        face[cOffsetPose + 2] = static_cast<float>(result.rpy[2]);
        face[cOffsetPose + 3] = static_cast<float>(result.position3D[0]);
        face[cOffsetPose + 4] = static_cast<float>(result.position3D[1]);
        face[cOffsetPose + 5] = static_cast<float>(result.position3D[2]);
      }

      return faceCount;
    }
  }
}
