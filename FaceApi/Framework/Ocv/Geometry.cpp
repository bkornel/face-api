#include "Framework/Ocv/Geometry.h"

#include <algorithm>

namespace fw
{
  namespace ocv
  {
    float overlap_ratio(const cv::Rect& iR1, const cv::Rect& iR2)
    {
      if (iR1.area() == 0 || iR2.area() == 0)
        return 0.0F;

      const cv::Rect& intersection = (iR1 & iR2);
      return (std::max)(static_cast<float>(intersection.area()) / static_cast<float>(iR1.area()),
                        static_cast<float>(intersection.area()) / static_cast<float>(iR2.area()));
    }

    void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation)
    {
      if (iRotation == 90) // transpose + flip(1) = CW
      {
        oOutput = iInput.t();          // Transpose original
        cv::flip(oOutput, oOutput, 1); // Flipping around the y axis
      }
      else if (iRotation == 180) // flip(-1) = 180
      {
        cv::flip(iInput, oOutput, -1); // Flipping around both axis
      }
      else if (iRotation == 270) // transpose + flip(0) = CCW
      {
        oOutput = iInput.t();           // Transpose original
        cv::flip(oOutput, oOutput, -1); // Flipping around both axis
      }
    }
  }
}
