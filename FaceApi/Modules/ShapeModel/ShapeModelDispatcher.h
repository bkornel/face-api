#pragma once

#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/IShapeFitter.h"
#include "Modules/ShapeModel/ShapeModel.h"

#include <map>
#include <memory>
#include <vector>

namespace face
{
  /// @brief The CLM fitter: one constrained local model per track, warm-started from where
  /// its track stood at the last fit. Precise near frontal, degrades past ~30 degrees of yaw.
  class ShapeModelDispatcher : public IShapeFitter
  {
  public:
    ShapeModelDispatcher() = default;

    virtual ~ShapeModelDispatcher() = default;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

    void BeginFrame(const std::vector<TrackedFace>& iTracks) override;

    bool Fit(const TrackedFace& iTrack, const cv::Mat& iFrameGray, const cv::Mat& iFrameBGR, ShapeDescriptor& oShape) override;

    void Clear() override;

  private:
    /// @brief A model and where its track stood at the last fit. The next warm start shifts
    /// the shape by how far the track moved since then - measured between the tracker's own
    /// rectangles, so the different framing of tracker box and fitted box cancels out.
    struct TrackModel
    {
      std::shared_ptr<ShapeModel> model;
      cv::Rect lastTrackRect;
      bool hasFit = false;
    };

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
