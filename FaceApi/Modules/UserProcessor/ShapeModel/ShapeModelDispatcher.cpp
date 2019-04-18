#include "Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.h"

#include "Framework/UtilString.h"
#include "Common/Configuration.h"
#include "User/User.h"

namespace face
{
  ShapeModelDispatcher::ShapeModels ShapeModelDispatcher::sShapeModels;

  ShapeModelDispatcher::~ShapeModelDispatcher()
  {
    fw::remove_if(sShapeModels, [&](const ShapeModels::value_type& obj)
    {
      return std::find_if(mUpdatedUserIDs.begin(), mUpdatedUserIDs.end(), [&](int id)
      {
        return obj.first == id;
      }) == mUpdatedUserIDs.end();
    });
  }

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
          mWinDetection.push_back(fw::str::convert_to_number<int>(itToken));
      }

      if (fw::ocv::get_value(iSettings, "winTracking", value))
      {
        mWinTracking.clear();

        auto tokens = fw::str::split(value, ',');
        for (auto itToken : tokens)
          mWinTracking.push_back(fw::str::convert_to_number<int>(itToken));
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

  bool ShapeModelDispatcher::Dispatch(User& ioUser)
  {
    assert(!mFrame.empty() && mFrame.type() == CV_8UC1);

    const int userId = ioUser.GetUserId();
    auto it = sShapeModels.find(userId);

    if (it == sShapeModels.end())
    {
      mShapeModel = std::make_shared<ShapeModel>();
      sShapeModels.insert(std::make_pair(userId, mShapeModel));
    }
    else
    {
      mShapeModel = it->second;
    }

    mUpdatedUserIDs.push_back(userId);

    return Fit(ioUser);
  }

  bool ShapeModelDispatcher::Fit(User& ioUser)
  {
    const auto& faceRect = ioUser.GetFaceRect();

    std::vector<int> winSize;

    if (ioUser.IsDetected())
    {
      winSize = mWinDetection;
      mShapeModel->InitShape(faceRect);
    }
    else
    {
      winSize = mWinTracking;
      mShapeModel->ShiftShape(ioUser.GetFaceRectOffset());
    }

    mShapeModel->Fit(mFrame, winSize, mNoIter, mClamp, mFTol);

    if (mFailureCheck && !mShapeModel->FailureCheck(mFrame)) return false;

    if (!UpdateTemplate(ioUser)) return false;

    return true;
  }

  bool ShapeModelDispatcher::UpdateTemplate(User& ioUser)
  {
    const cv::Rect screenRect(0, 0, mFrame.cols, mFrame.rows);

    cv::Point2d minPt;
    cv::Point2d maxPt;
    if (!mShapeModel->GetMinMax2D(screenRect, minPt, maxPt)) return false;

    const cv::Rect newFaceRect(minPt, maxPt);
    if (newFaceRect.area() <= 0) return false;

    const cv::Mat& shape2DMat = mShapeModel->GetShape2D();
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
