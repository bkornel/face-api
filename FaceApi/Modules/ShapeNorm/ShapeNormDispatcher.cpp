#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/ShapeNorm/ShapeNormDispatcher.h"
#include "Model/FaceModel.h"

#include "Framework/Text.h"

#include <opencv2/core/core.hpp>

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

  fw::VectorPt2D ShapeNormDispatcher::Normalize2D(const fw::VectorPt2D& iShape2D) const
  {
    const auto& shape2D = cv::Mat(iShape2D);
    const auto& refShape2D = cv::Mat(FaceModel::GetInstance().GetFrontalShape2D());

    // The reference is the second shape and the mean is seeded from it, so what the
    // alignment converges on is the reference's own frame
    fw::ShapeVector shapes2D = { shape2D.clone(), refShape2D.clone() };
    cv::Mat meanShape2D = refShape2D.clone().reshape(1);
    fw::generalized_procrustes(shapes2D, meanShape2D, mMaxIterations, mEpsilon);

    // The aligned shape, not the mean the alignment converged on: the mean of a face and
    // the reference is a face halfway to the model, which is nobody's face
    fw::VectorPt2D result;
    shapes2D.front().reshape(2).copyTo(result);
    return result;
  }

  fw::VectorPt3D ShapeNormDispatcher::Normalize3D(const fw::VectorPt3D& iShape3D) const
  {
    const auto& shape3D = cv::Mat(iShape3D);
    const auto& refShape3D = cv::Mat(FaceModel::GetInstance().GetShape3D());

    fw::ShapeVector shapes3D = { shape3D.clone(), refShape3D.clone() };
    cv::Mat meanShape3D = refShape3D.clone().reshape(1);
    fw::generalized_procrustes(shapes3D, meanShape3D, mMaxIterations, mEpsilon);

    fw::VectorPt3D result;
    shapes3D.front().reshape(3).copyTo(result);
    return result;
  }
}
