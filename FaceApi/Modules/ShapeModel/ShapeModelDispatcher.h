#pragma once

#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/ShapeModel.h"

#include <map>
#include <memory>
#include <vector>

namespace face
{
  class User;

  /// @brief Owns one ShapeModel per tracked user and fits them to a frame. GetModel() and
  /// RetainModels() maintain the map and stay on one thread; Fit() touches only the model
  /// and user it is given, so different users may be fitted concurrently.
  class ShapeModelDispatcher
  {
    using ShapeModels = std::map<int, std::shared_ptr<ShapeModel>>;

  public:
    ShapeModelDispatcher() = default;

    ShapeModelDispatcher(const ShapeModelDispatcher& iOther) = delete;

    ShapeModelDispatcher& operator=(const ShapeModelDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    std::shared_ptr<ShapeModel> GetModel(const User& iUser);

    void RetainModels(const std::vector<std::shared_ptr<User>>& iUsers);

    bool Fit(User& ioUser, ShapeModel& ioShapeModel, const cv::Mat& iFrame) const;

  private:
    bool UpdateTemplate(User& ioUser, ShapeModel& ioShapeModel, const cv::Mat& iFrame) const;

    ShapeModels mShapeModels;

    std::vector<int> mWinDetection = { 11, 9, 7 };
    std::vector<int> mWinTracking = { 7 };
    int mNoIter = 10;
    double mClamp = 3.0;
    double mFTol = 0.01;
    bool mFailureCheck = false;
  };
}
