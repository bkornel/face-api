#pragma once

#include "User/UserDispatcher.hpp"
#include "Modules/UserProcessor/ShapeModel/ShapeModel.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace face
{
  class User;
  class ShapeModel;
  class UserData;

  class ShapeModelDispatcher : public UserDispatcher
  {
    typedef std::map<int, std::shared_ptr<ShapeModel>> ShapeModels;

  public:
    ShapeModelDispatcher() = default;

    virtual ~ShapeModelDispatcher() = default;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

    bool Dispatch(User& ioUser) override;

    void BeginFrame(const cv::Mat& iFrame);

    void EndFrame();

  private:
    bool Fit(User& ioUser, ShapeModel& ioShapeModel);

    bool UpdateTemplate(User& ioUser, ShapeModel& ioShapeModel);

    ShapeModels mShapeModels;
    std::set<int> mDispatchedUserIDs;

    cv::Mat mFrame;
    std::vector<int> mWinDetection = { 11, 9, 7 };
    std::vector<int> mWinTracking = { 7 };
    int mNoIter = 10;
    double mClamp = 3.0;
    double mFTol = 0.01;
    bool mFailureCheck = false;
  };
}
