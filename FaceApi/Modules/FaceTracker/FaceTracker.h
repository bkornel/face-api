#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"
#include "Framework/Stopwatch.h"

#include "Messages/FaceTrackMessage.h"
#include "Messages/ImageMessage.h"
#include "Messages/RoiMessage.h"
#include "User/TrackedFace.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace face
{
  /// @brief Follows the faces on the frame and owns their identity: merges the detector's
  /// rectangles into the known tracks, follows undetected tracks by template matching and
  /// retires the ones that have been away too long. Everything it publishes is a value -
  /// the modules downstream compute with the tracks, they never write into them.
  class FaceTracker : public fw::Module,
                      public fw::Port<std::shared_ptr<FaceTrackMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<RoiMessage>)>
  {
  public:

    FaceTracker() = default;

    virtual ~FaceTracker() = default;

    /// @brief Wired by whoever builds the graph; called when the tracker has a free slot
    /// and wants the detector to look for a face to fill it
    void SetDetectionRequest(std::function<void()> iRequest)
    {
      mRequestDetection = std::move(iRequest);
    }

    std::shared_ptr<FaceTrackMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<RoiMessage> iDetections) override;

    void Clear() override;

    inline std::size_t GetMaxTracks() const
    {
      return mMaxTracks;
    }

  private:
    /// @brief A track and what following it needs: the appearance at its rectangle
    struct Track
    {
      TrackedFace face;
      cv::Mat faceTemplate;
    };

    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void PreprocessTracks(fw::Timestamp iTimestamp);

    void ProcessDetections(std::shared_ptr<RoiMessage> iDetections, fw::Timestamp iTimestamp);

    void MergeDetectionsAndTracks(std::vector<cv::Rect>& ioFaceROIs, fw::Timestamp iTimestamp);

    void FollowTracks(std::shared_ptr<ImageMessage> iImage);

    bool MatchTemplate(std::shared_ptr<ImageMessage> iImage, const Track& iTrack, cv::Rect& oFaceRect);

    void RemoveInactiveTracks(bool iForceToDelete = false);

    void SetStatus(Track& ioTrack, TrackStatus iStatus);

    std::size_t GetActiveTrackCount() const;

    std::function<void()> mRequestDetection;

    std::vector<Track> mTracks;
    fw::Stopwatch mRemoveSW;
    int mNextTrackId = 0;

    cv::Size mMinFaceSize;
    cv::Size mMaxFaceSize;

    std::size_t mMaxTracks = 1U;
    float mTrackOverlap = 0.2F;
    float mTrackAwaySec = 15.0F;
    float mTemplateScale = 1.0F;
    float mTemplateScaleInv = 1.0F;
  };
}
