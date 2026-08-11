#include "Framework/UtilContainer.h"
#include "Framework/ErrorCode.h"
#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"

#include "Framework/UtilString.h"
#include "Common/Configuration.h"
#include "User/User.h"

namespace face
{
  fw::ErrorCode ShapeModelDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      if (fw::ocv::get_value(iSettings, "winDetection", value))
      {
        mWinDetection.clear();

        auto tokens = fw::str::split(value, ',');
        for (auto itToken : tokens)
          mWinDetection.emplace_back(fw::str::convert_to_number<int>(itToken));
      }

      if (fw::ocv::get_value(iSettings, "winTracking", value))
      {
        mWinTracking.clear();

        auto tokens = fw::str::split(value, ',');
        for (auto itToken : tokens)
          mWinTracking.emplace_back(fw::str::convert_to_number<int>(itToken));
      }

      if (fw::ocv::get_value(iSettings, "nIter", value))
        mNoIter = fw::str::convert_to_number<int>(value);

      if (fw::ocv::get_value(iSettings, "clamp", value))
        mClamp = fw::str::convert_to_number<float>(value);

      if (fw::ocv::get_value(iSettings, "fTol", value))
        mFTol = fw::str::convert_to_number<float>(value);

      if (fw::ocv::get_value(iSettings, "failureCheck", value))
        mFailureCheck = fw::str::convert_to_boolean(value);
    }

    return fw::ErrorCode::OK;
  }

  void ShapeModelDispatcher::BeginFrame(const cv::Mat& iFrame)
  {
    mFrame = iFrame;
    mDispatchedUserIDs.clear();
  }

  void ShapeModelDispatcher::EndFrame()
  {
    // Drop the models of the users that are no longer around. A user that comes back is
    // Detected again, and Fit() re-initializes its shape from the face rectangle in that
    // case, so there is nothing in the old model worth keeping.
    fw::remove_if(mShapeModels, [&](const ShapeModels::value_type& obj) {
      return mDispatchedUserIDs.find(obj.first) == mDispatchedUserIDs.end();
    });
  }

  bool ShapeModelDispatcher::Dispatch(User& ioUser)
  {
    CV_DbgAssert(!mFrame.empty() && mFrame.type() == CV_8UC1);

    const int userId = ioUser.GetUserId();
    mDispatchedUserIDs.insert(userId);

    auto it = mShapeModels.find(userId);
    if (it == mShapeModels.end())
    {
      it = mShapeModels.emplace(userId, std::make_shared<ShapeModel>()).first;
    }

    return Fit(ioUser, *(it->second));
  }

  bool ShapeModelDispatcher::Fit(User& ioUser, ShapeModel& ioShapeModel)
  {
    const auto& faceRect = ioUser.GetFaceRect();

    std::vector<int> winSize;

    if (ioUser.IsDetected())
    {
      winSize = mWinDetection;
      ioShapeModel.InitShape(faceRect);
    }
    else
    {
      winSize = mWinTracking;
      ioShapeModel.ShiftShape(ioUser.GetFaceRectOffset());
    }

    ioShapeModel.Fit(mFrame, winSize, mNoIter, mClamp, mFTol);

    if (mFailureCheck && !ioShapeModel.FailureCheck(mFrame)) return false;

    return UpdateTemplate(ioUser, ioShapeModel);
  }

  bool ShapeModelDispatcher::UpdateTemplate(User& ioUser, ShapeModel& ioShapeModel)
  {
    const cv::Rect screenRect(0, 0, mFrame.cols, mFrame.rows);

    cv::Point2d minPt;
    cv::Point2d maxPt;
    if (!ioShapeModel.GetMinMax2D(screenRect, minPt, maxPt)) return false;

    const cv::Rect newFaceRect(minPt, maxPt);
    if (newFaceRect.area() <= 0) return false;

    const cv::Mat& shape2DMat = ioShapeModel.GetShape2D();
    const int count = shape2DMat.rows / 2;
    fw::ocv::VectorPt2D shape2D(count);

    for (int i = 0; i < count; i++)
      shape2D[i] = { shape2DMat.at<double>(i, 0), shape2DMat.at<double>(i + count, 0) };

    ioUser.SetShape2D(shape2D);
    ioUser.SetFaceRect(newFaceRect);
    ioUser.SetFaceTemplate(mFrame(newFaceRect));

    return true;
  }
}
