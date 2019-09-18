#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/ShapeModel/ClmWrapper.h"

#include "Framework/UtilString.h"
#include "User/User.h"

#include <opencv2/core/core.hpp>
#include <numeric>

namespace face
{
  fw::ErrorCode ShapeNormDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::ocv::get_value(iSettings, "maxCount", value))
        mMaxCount = fw::str::convert_to_number<int>(value);

      if (fw::ocv::get_value(iSettings, "epsilon", value))
        mEpsilon = fw::str::convert_to_number<double>(value);
    }

    return fw::ErrorCode::OK;
  }

  bool ShapeNormDispatcher::Dispatch(User& ioUser)
  {
    mMeanShape2D.clear();
    mMeanShape3D.clear();

    NormalizeShape2D(ioUser);
    NormalizeShape3D(ioUser);

    ioUser.SetNormShapes(mMeanShape2D, mMeanShape3D);

    return true;
  }

  void ShapeNormDispatcher::NormalizeShape2D(User& ioUser)
  {
    const auto& userShape2D = cv::Mat(ioUser.GetShape2D());
    const auto& refShape2D = cv::Mat(ClmWrapper::GetInstance().GetReferenceShape2D());

    MatVector shapes2D = { userShape2D.clone(), refShape2D.clone() };
    cv::Mat meanShape2D = refShape2D.clone().reshape(1);
    GPA(shapes2D, meanShape2D);

    meanShape2D.reshape(2).copyTo(mMeanShape2D);
  }

  void ShapeNormDispatcher::NormalizeShape3D(User& ioUser)
  {
    const auto& userShape3D = cv::Mat(ioUser.GetShape3D());
    const auto& refShape3D = cv::Mat(ClmWrapper::GetInstance().GetReferenceShape3D());

    MatVector shapes3D = { userShape3D.clone(), refShape3D.clone() };
    cv::Mat meanShape3D = refShape3D.clone().reshape(1);
    GPA(shapes3D, meanShape3D);

    meanShape3D.reshape(3).copyTo(mMeanShape3D);
  }

  void ShapeNormDispatcher::GPA(MatVector& ioShapes2D, cv::Mat& ioMeanShape) const
  {
    int counter = 0;
    while (true)
    {
      // recenter, normalize, align
      Recenter(ioShapes2D);
      Normalize(ioShapes2D);
      Align(ioShapes2D, ioMeanShape);

      // Find a new mean shape from all the set of points
      cv::Mat newMeanShape = cv::Mat::zeros(ioMeanShape.size(), ioMeanShape.type());
      std::accumulate(ioShapes2D.begin(), ioShapes2D.end(), newMeanShape);

      newMeanShape = newMeanShape / ioShapes2D.size();
      newMeanShape = newMeanShape / cv::norm(newMeanShape);

      // Perform the loop until convergence
      const double diff = cv::norm(newMeanShape, ioMeanShape);
      if (counter++ > mMaxCount || diff <= mEpsilon)
        break;

      ioMeanShape = newMeanShape;
    }
  }

  void ShapeNormDispatcher::Recenter(MatVector& ioShapes) const
  {
    for (auto& shape : ioShapes)
    {
      const cv::Scalar& mean = cv::mean(shape);
      shape = shape - cv::Mat(shape.size(), shape.type(), mean);
    }
  }

  void ShapeNormDispatcher::Normalize(MatVector& ioShapes) const
  {
    for (auto& shape : ioShapes)
      shape = shape / cv::norm(shape);
  }

  void ShapeNormDispatcher::Align(MatVector& ioShapes, cv::Mat& ioMeanShape) const
  {
    cv::Mat w, u, vt;
    for (auto& shape : ioShapes)
    {
      cv::SVDecomp(ioMeanShape.reshape(1).t() * shape.reshape(1), w, u, vt);
      shape = (shape.reshape(1) * vt.t()) * u.t();
    }
  }
}
