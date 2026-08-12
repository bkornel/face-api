#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"

#include "Framework/Text.h"
#include "User/User.h"

#include <algorithm>

namespace face
{
  fw::ErrorCode ShapeModelDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "winDetection", value))
      {
        mWinDetection.clear();

        auto tokens = fw::str::split(value, ',');
        for (auto itToken : tokens)
          mWinDetection.emplace_back(fw::str::convert_to_number<int>(itToken));
      }

      if (fw::get_value(iSettings, "winTracking", value))
      {
        mWinTracking.clear();

        auto tokens = fw::str::split(value, ',');
        for (auto itToken : tokens)
          mWinTracking.emplace_back(fw::str::convert_to_number<int>(itToken));
      }

      if (fw::get_value(iSettings, "nIter", value))
        mNoIter = fw::str::convert_to_number<int>(value);

      if (fw::get_value(iSettings, "clamp", value))
        mClamp = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "fTol", value))
        mFTol = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "failureCheck", value))
        mFailureCheck = fw::str::convert_to_boolean(value);
    }

    return fw::ErrorCode::OK;
  }

  std::shared_ptr<ShapeModel> ShapeModelDispatcher::GetModel(const User& iUser)
  {
    auto it = mShapeModels.find(iUser.GetUserId());
    if (it == mShapeModels.end())
    {
      it = mShapeModels.emplace(iUser.GetUserId(), std::make_shared<ShapeModel>()).first;
    }

    return it->second;
  }

  void ShapeModelDispatcher::RetainModels(const std::vector<std::shared_ptr<User>>& iUsers)
  {
    // Drop the models of the users that are no longer around. A user that comes back is
    // Detected again, and Fit() re-initializes its shape from the face rectangle then.
    std::erase_if(mShapeModels, [&iUsers](const ShapeModels::value_type& iEntry) {
      return std::none_of(iUsers.begin(), iUsers.end(), [&iEntry](const std::shared_ptr<User>& iUser) {
        return iUser && iUser->GetUserId() == iEntry.first;
      });
    });
  }

  bool ShapeModelDispatcher::Fit(User& ioUser, ShapeModel& ioShapeModel, const cv::Mat& iFrame) const
  {
    // Copied because the fit takes a mutable reference; the members stay read-only, which
    // is what lets different users run concurrently through this method
    std::vector<int> winSize;

    if (ioUser.IsDetected())
    {
      winSize = mWinDetection;
      ioShapeModel.InitShape(ioUser.GetFaceRect());
    }
    else
    {
      winSize = mWinTracking;
      ioShapeModel.ShiftShape(ioUser.GetFaceRectOffset());
    }

    ioShapeModel.Fit(iFrame, winSize, mNoIter, mClamp, mFTol);

    if (mFailureCheck && !ioShapeModel.FailureCheck(iFrame)) return false;

    return UpdateTemplate(ioUser, ioShapeModel, iFrame);
  }

  bool ShapeModelDispatcher::UpdateTemplate(User& ioUser, ShapeModel& ioShapeModel, const cv::Mat& iFrame) const
  {
    const cv::Rect screenRect(0, 0, iFrame.cols, iFrame.rows);

    cv::Point2d minPt;
    cv::Point2d maxPt;
    if (!ioShapeModel.GetMinMax2D(screenRect, minPt, maxPt)) return false;

    const cv::Rect newFaceRect(minPt, maxPt);
    if (newFaceRect.area() <= 0) return false;

    const cv::Mat& shape2DMat = ioShapeModel.GetShape2D();
    const int count = shape2DMat.rows / 2;
    fw::VectorPt2D shape2D(count);

    for (int i = 0; i < count; i++)
      shape2D[i] = { shape2DMat.at<double>(i, 0), shape2DMat.at<double>(i + count, 0) };

    ioUser.SetShape2D(shape2D);
    ioUser.SetFaceRect(newFaceRect);
    ioUser.SetFaceTemplate(iFrame(newFaceRect));

    return true;
  }
}
