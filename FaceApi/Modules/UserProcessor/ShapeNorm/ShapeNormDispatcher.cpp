#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.h"
#include "Modules/UserProcessor/ShapeModel/ClmWrapper.h"

#include "Framework/Text.h"
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

      if (fw::get_value(iSettings, "maxCount", value))
        mMaxIterations = fw::str::convert_to_number<int>(value);

      if (fw::get_value(iSettings, "epsilon", value))
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

    fw::ShapeVector shapes2D = { userShape2D.clone(), refShape2D.clone() };
    cv::Mat meanShape2D = refShape2D.clone().reshape(1);
    fw::generalized_procrustes(shapes2D, meanShape2D, mMaxIterations, mEpsilon);

    meanShape2D.reshape(2).copyTo(mMeanShape2D);
  }

  void ShapeNormDispatcher::NormalizeShape3D(User& ioUser)
  {
    const auto& userShape3D = cv::Mat(ioUser.GetShape3D());
    const auto& refShape3D = cv::Mat(ClmWrapper::GetInstance().GetReferenceShape3D());

    fw::ShapeVector shapes3D = { userShape3D.clone(), refShape3D.clone() };
    cv::Mat meanShape3D = refShape3D.clone().reshape(1);
    fw::generalized_procrustes(shapes3D, meanShape3D, mMaxIterations, mEpsilon);

    meanShape3D.reshape(3).copyTo(mMeanShape3D);
  }

}
