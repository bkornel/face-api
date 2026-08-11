#include "Framework/Imaging/Transform.h"

namespace fw
{
  void rotate_mat(const cv::Mat& iInput, cv::Mat& oOutput, int iRotation)
  {
    // Rotated into a local first: the caller may pass the same matrix as input and output,
    // and cv::rotate transposes for the 90 and 270 cases, which changes the dimensions.
    cv::Mat rotated;

    switch (iRotation)
    {
      case 90:
        cv::rotate(iInput, rotated, cv::ROTATE_90_CLOCKWISE);
        break;

      case 180:
        cv::rotate(iInput, rotated, cv::ROTATE_180);
        break;

      case 270:
        cv::rotate(iInput, rotated, cv::ROTATE_90_COUNTERCLOCKWISE);

        // REMARK: this mirrors as well, which a 270 degree rotation does not. It is what the
        // previous transpose + flip(-1) implementation did, so it is kept rather than
        // quietly changed - the front camera path may well depend on it. Drop this flip for
        // a plain rotation.
        cv::flip(rotated, rotated, 1);
        break;

      default:
        return;
    }

    oOutput = rotated;
  }
}
