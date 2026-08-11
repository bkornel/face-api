#include "Framework/Imaging/Statistics.h"

namespace fw
{
  double sum_squared(const cv::Mat& iMatrix)
  {
    CV_DbgAssert(!iMatrix.empty() && (iMatrix.channels() == 1 || iMatrix.channels() == 2 || iMatrix.channels() == 3));

    cv::Mat pow;
    cv::pow(iMatrix, 2.0, pow);

    const cv::Scalar& sum = cv::sum(pow);
    return sum[0] + sum[1] + sum[2];
  }
}
