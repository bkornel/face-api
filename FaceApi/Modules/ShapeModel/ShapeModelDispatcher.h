#pragma once

#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/ShapeModel.h"
#include "User/TrackedFace.h"
#include "Messages/ShapeMessage.h"

#include <map>
#include <memory>
#include <vector>

namespace face
{
  /// @brief Owns one ShapeModel per track and fits them to a frame. The model map and the
  /// per-track fit anchors are maintained on one thread through GetModel()/RetainModels();
  /// Fit() touches only the model it is given, so different tracks may run concurrently.
  class ShapeModelDispatcher
  {
  public:
    /// @brief A model and where its track stood at the last fit. The next warm start shifts
    /// the shape by how far the track moved since then - measured between the tracker's own
    /// rectangles, so the different framing of tracker box and fitted box cancels out.
    struct TrackModel
    {
      std::shared_ptr<ShapeModel> model;
      cv::Rect lastTrackRect;
      bool hasFit = false;
    };

    ShapeModelDispatcher() = default;

    ShapeModelDispatcher(const ShapeModelDispatcher& iOther) = delete;

    ShapeModelDispatcher& operator=(const ShapeModelDispatcher& iOther) = delete;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings);

    TrackModel& GetModel(const TrackedFace& iTrack);

    void RetainModels(const std::vector<TrackedFace>& iTracks);

    void Clear();

    /// @brief Fits one track and fills oData with the shape and the refined rectangle.
    /// Reentrant across distinct tracks. ioModel's anchor is written by the one thread
    /// that owns this track's fit.
    bool Fit(const TrackedFace& iTrack, TrackModel& ioModel, const cv::Mat& iFrame, ShapeDescriptor& oShape) const;

  private:
    using TrackModels = std::map<int, TrackModel>;

    TrackModels mModels;

    std::vector<int> mWinDetection = { 11, 9, 7 };
    std::vector<int> mWinTracking = { 7 };
    int mNoIter = 10;
    double mClamp = 3.0;
    double mFTol = 0.01;
    bool mFailureCheck = false;
  };
}
