#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/ShapeModelDispatcher.h"

#include "Framework/Text.h"

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

  ShapeModelDispatcher::TrackModel& ShapeModelDispatcher::GetModel(const TrackedFace& iTrack)
  {
    auto it = mModels.find(iTrack.trackId);
    if (it == mModels.end())
    {
      it = mModels.emplace(iTrack.trackId, TrackModel{ std::make_shared<ShapeModel>(), {}, false }).first;
    }

    return it->second;
  }

  void ShapeModelDispatcher::RetainModels(const std::vector<TrackedFace>& iTracks)
  {
    // Drop the models of the tracks that are no longer around. A track that comes back is
    // Detected again, and Fit() re-initializes its shape from the face rectangle then.
    std::erase_if(mModels, [&iTracks](const TrackModels::value_type& iEntry) {
      return std::none_of(iTracks.begin(), iTracks.end(), [&iEntry](const TrackedFace& iTrack) {
        return iTrack.trackId == iEntry.first;
      });
    });
  }

  void ShapeModelDispatcher::Clear()
  {
    mModels.clear();
  }

  bool ShapeModelDispatcher::Fit(const TrackedFace& iTrack, TrackModel& ioModel, const cv::Mat& iFrame, ShapeDescriptor& oShape) const
  {
    ShapeModel& shapeModel = *ioModel.model;

    // Copied because the fit takes a mutable reference; the members stay read-only, which
    // is what lets different tracks run concurrently through this method
    std::vector<int> winSize;

    if (iTrack.status == TrackStatus::Detected || !ioModel.hasFit)
    {
      winSize = mWinDetection;
      shapeModel.InitShape(iTrack.faceRect);
    }
    else
    {
      // The model sits where its last fit converged; move it by how far the track moved
      winSize = mWinTracking;
      shapeModel.ShiftShape(iTrack.faceRect.tl() - ioModel.lastTrackRect.tl());
    }

    // Where the track stood when this fit ran, which is what the next shift measures from
    ioModel.lastTrackRect = iTrack.faceRect;
    ioModel.hasFit = true;

    shapeModel.Fit(iFrame, winSize, mNoIter, mClamp, mFTol);

    const cv::Mat& shape2DMat = shapeModel.GetShape2D();
    const int count = shape2DMat.rows / 2;
    fw::VectorPt2D shape2D(count);

    cv::Point2d minPt = { shape2DMat.at<double>(0, 0), shape2DMat.at<double>(count, 0) };
    cv::Point2d maxPt = minPt;

    for (int i = 0; i < count; i++)
    {
      shape2D[i] = { shape2DMat.at<double>(i, 0), shape2DMat.at<double>(i + count, 0) };

      if (cvIsNaN(shape2D[i].x) || cvIsInf(shape2D[i].x) || cvIsNaN(shape2D[i].y) || cvIsInf(shape2D[i].y))
        return false;

      minPt.x = (std::min)(minPt.x, shape2D[i].x);
      minPt.y = (std::min)(minPt.y, shape2D[i].y);
      maxPt.x = (std::max)(maxPt.x, shape2D[i].x);
      maxPt.y = (std::max)(maxPt.y, shape2D[i].y);
    }

    const cv::Rect fittedRect(minPt, maxPt);

    if (mFailureCheck && !shapeModel.FailureCheck(iFrame)) return false;

    // Only a shape that lies fully on the frame is worth reporting
    const cv::Rect screenRect(0, 0, iFrame.cols, iFrame.rows);
    if (!screenRect.contains(minPt) || !screenRect.contains(maxPt) || fittedRect.area() <= 0) return false;

    oShape.trackId = iTrack.trackId;
    oShape.faceRect = fittedRect;
    oShape.shape2D = std::move(shape2D);

    return true;
  }
}
