#include "Framework/Imaging/MatExtensions.h"

namespace fw
{
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

  double sum_squared(const cv::Mat& iMatrix)
  {
    CV_DbgAssert(!iMatrix.empty() && (iMatrix.channels() == 1 || iMatrix.channels() == 2 || iMatrix.channels() == 3));

    cv::Mat pow;
    cv::pow(iMatrix, 2.0, pow);

    const cv::Scalar& sum = cv::sum(pow);
    return sum[0] + sum[1] + sum[2];
  }
}
