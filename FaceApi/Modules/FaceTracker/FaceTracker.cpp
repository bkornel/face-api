#include "Framework/Imaging/Geometry.h"
#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Framework/TimeExtensions.h"
#include "Modules/FaceTracker/FaceTracker.h"
#include "Messages/CommandMessage.h"

#include "Framework/Profiler.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>

namespace face
{
  fw::ErrorCode FaceTracker::InitializeInternal(const cv::FileNode& iSettings)
  {
    if (!iSettings.empty())
    {
      std::string value;

      // At least one: a graph that may track nobody at all has no reason to run.
      // The old names are still read so existing settings files keep working.
      if (fw::get_value(iSettings, "maxTracks", value) || fw::get_value(iSettings, "maxUsers", value))
        mMaxTracks = static_cast<std::size_t>((std::max)(fw::str::convert_to_number<int>(value), 1));

      if (fw::get_value(iSettings, "trackOverlap", value) || fw::get_value(iSettings, "userOverlap", value))
        mTrackOverlap = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "trackAwaySec", value) || fw::get_value(iSettings, "userAwaySec", value))
        mTrackAwaySec = fw::str::convert_to_number<float>(value);

      if (fw::get_value(iSettings, "templateScale", value))
      {
        mTemplateScale = fw::str::convert_to_number<float>(value);
        mTemplateScale = (std::max)((std::min)(mTemplateScale, 1.0F), 0.2F);
        mTemplateScaleInv = (1.0F / mTemplateScale);
      }
    }

    mRemoveSW.Start();

    return fw::ErrorCode::OK;
  }

  void FaceTracker::Clear()
  {
    for (auto& track : mTracks)
      SetStatus(track, TrackStatus::Inactive);

    RemoveInactiveTracks(true);
    mRemoveSW.Reset();

    mMinFaceSize = mMaxFaceSize = { 0, 0 };
  }

  std::shared_ptr<FaceTrackMessage> FaceTracker::Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<RoiMessage> iDetections)
  {
    DrainCommands();

    if (!iImage || iImage->IsEmpty()) return nullptr;

    FACE_PROFILER(Face_Tracker);

    const uint32_t frameId = iImage->GetFrameId();
    const fw::Timestamp timestamp = iImage->GetTimestamp();

    // Active tracks to inactive if they aged or left the allowed size range
    PreprocessTracks(timestamp);

    // Merge detections into tracks and start new tracks
    ProcessDetections(iDetections, timestamp);

    // Follow the tracks the detector did not confirm this frame
    FollowTracks(iImage);

    // Remove the tracks that have been inactive for long
    if (mRemoveSW.GetElapsedTimeSec(false) > mTrackAwaySec)
    {
      RemoveInactiveTracks();
      mRemoveSW.Reset();
    }

    if (GetMaxTracks() != GetActiveTrackCount())
    {
      Publish(std::make_shared<CommandMessage>(CommandMessage::Type::RunFaceDetection, frameId, timestamp));
    }

    FaceTrackMessage::TrackVector tracks;
    tracks.reserve(mTracks.size());

    for (const auto& track : mTracks)
    {
      if (track.face.status != TrackStatus::Inactive)
        tracks.emplace_back(track.face);
    }

    return tracks.empty() ? nullptr : std::make_shared<FaceTrackMessage>(std::move(tracks), frameId, timestamp);
  }

  void FaceTracker::PreprocessTracks(fw::Timestamp iTimestamp)
  {
    for (auto& track : mTracks)
    {
      const auto& faceRect = track.face.faceRect;

      const bool inactivate =
        // Must be active
        track.face.status != TrackStatus::Inactive && (
          // Minimal resolution
          (!mMinFaceSize.empty() && ((faceRect.width < mMinFaceSize.width) || (faceRect.height < mMinFaceSize.height))) ||
          // Maximal resolution
          (!mMaxFaceSize.empty() && ((faceRect.width > mMaxFaceSize.width) || (faceRect.height > mMaxFaceSize.height))) ||
          // Detected a long time ago
          (fw::elapsed(track.face.lastDetectionTs, iTimestamp) > fw::Milliseconds(mTrackAwaySec * 1000.0F))
        );

      if (inactivate)
        SetStatus(track, TrackStatus::Inactive);

      // A detection only holds for the frame it happened on
      if (track.face.status == TrackStatus::Detected)
        SetStatus(track, TrackStatus::Tracked);
    }
  }

  void FaceTracker::ProcessDetections(std::shared_ptr<RoiMessage> iDetections, fw::Timestamp iTimestamp)
  {
    if (!iDetections || iDetections->IsEmpty()) return;

    std::vector<cv::Rect> faceROIs = iDetections->GetROIs();
    mMinFaceSize = iDetections->GetMinRoiSize();
    mMaxFaceSize = iDetections->GetMaxRoiSize();

    // Checking the overlap between the detector's rectangles and the tracks
    MergeDetectionsAndTracks(faceROIs, iTimestamp);

    // Start new tracks from the detections that matched nobody
    for (const auto& roi : faceROIs)
    {
      if (GetActiveTrackCount() >= GetMaxTracks()) break;

      Track track;
      track.face.trackId = mNextTrackId;
      track.face.status = TrackStatus::Detected;
      track.face.faceRect = roi;
      track.face.creationTs = iTimestamp;
      track.face.lastDetectionTs = iTimestamp;
      track.face.lastUpdateTs = iTimestamp;

      mTracks.emplace_back(std::move(track));

      LOG(INFO) << "New user has been recognized, Welcome User(" << mNextTrackId << ")!";
      mNextTrackId++;
    }
  }

  void FaceTracker::MergeDetectionsAndTracks(std::vector<cv::Rect>& ioFaceROIs, fw::Timestamp iTimestamp)
  {
    for (auto& track : mTracks)
    {
      // Taking an inactive track back makes it active, so it needs a free slot. Asked per
      // track, not per detection: the answer cannot change while this track is matched.
      if (track.face.status == TrackStatus::Inactive && GetActiveTrackCount() >= GetMaxTracks())
        continue;

      // A copy: the assignment below moves the rectangle, and the detections are matched
      // against where the track was last seen.
      const cv::Rect trackRect = track.face.faceRect;

      for (auto roi = ioFaceROIs.begin(); roi != ioFaceROIs.end();)
      {
        if (fw::overlap_ratio(trackRect, *roi) > mTrackOverlap)
        {
          track.face.faceRect = *roi;
          track.face.lastDetectionTs = iTimestamp;
          SetStatus(track, TrackStatus::Detected);

          roi = ioFaceROIs.erase(roi);
        }
        else
        {
          ++roi;
        }
      }
    }
  }

  void FaceTracker::FollowTracks(std::shared_ptr<ImageMessage> iImage)
  {
    FACE_PROFILER(Follow_Tracks);

    const cv::Mat frameGray = iImage->GetFrameGray();
    const cv::Rect screenRect(0, 0, frameGray.cols, frameGray.rows);

    for (auto& track : mTracks)
    {
      if (track.face.status == TrackStatus::Inactive)
        continue;

      // A detected track sits on a fresh detector rectangle; the others are searched for
      if (track.face.status != TrackStatus::Detected)
      {
        cv::Rect newFaceRect;
        if (!MatchTemplate(iImage, track, newFaceRect))
        {
          SetStatus(track, TrackStatus::Inactive);
          continue;
        }

        track.face.faceRect = newFaceRect;
      }

      const cv::Rect faceRect = track.face.faceRect & screenRect;

      if (faceRect.area() <= 0)
      {
        SetStatus(track, TrackStatus::Inactive);
        continue;
      }

      track.faceTemplate = frameGray(faceRect).clone();
      track.face.lastUpdateTs = iImage->GetTimestamp();
    }
  }

  bool FaceTracker::MatchTemplate(std::shared_ptr<ImageMessage> iImage, const Track& iTrack, cv::Rect& oFaceRect)
  {
    CV_DbgAssert(mTemplateScale > 0.0F && mTemplateScale <= 1.0F);

    oFaceRect = {};

    if (iTrack.faceTemplate.empty()) return false;

    const cv::Mat& frame = iImage->GetResizedGray(mTemplateScale);
    cv::Mat faceTpl = iTrack.faceTemplate;

    if (std::abs(mTemplateScale - 1.0F) > std::numeric_limits<float>::epsilon())
      cv::resize(faceTpl, faceTpl, {}, mTemplateScale, mTemplateScale);

    if ((faceTpl.cols > frame.cols) || (faceTpl.rows > frame.rows))
      return false;

    cv::Mat result(frame.cols - faceTpl.cols + 1, frame.rows - faceTpl.rows + 1, CV_32FC1);
    cv::matchTemplate(frame, faceTpl, result, cv::TM_CCOEFF_NORMED);

    double maxVal = 0.0;
    cv::Point maxLoc;
    cv::minMaxLoc(result, nullptr, &maxVal, nullptr, &maxLoc);

    oFaceRect = {
      cvRound(maxLoc.x * mTemplateScaleInv),
      cvRound(maxLoc.y * mTemplateScaleInv),
      cvRound(faceTpl.cols * mTemplateScaleInv),
      cvRound(faceTpl.rows * mTemplateScaleInv)
    };

    const cv::Rect screenRect(0, 0, iImage->GetWidth(), iImage->GetHeight());
    oFaceRect = oFaceRect & screenRect;

    return oFaceRect.area() > 0;
  }

  void FaceTracker::RemoveInactiveTracks(bool iForceToDelete)
  {
    std::erase_if(mTracks, [&](const Track& iTrack) {
      if (iTrack.face.status != TrackStatus::Inactive) return false;

      const fw::Milliseconds idle = fw::elapsed_since(iTrack.face.lastUpdateTs);
      if (!iForceToDelete && idle <= fw::Milliseconds(mTrackAwaySec * 1000.0F)) return false;

      LOG(INFO) << "User(" << iTrack.face.trackId << ") has been deleted completely.";
      return true;
    });
  }

  void FaceTracker::SetStatus(Track& ioTrack, TrackStatus iStatus)
  {
    if (ioTrack.face.status == iStatus) return;

    ioTrack.face.status = iStatus;

    if (iStatus == TrackStatus::Detected)
    {
      LOG(INFO) << "User(" << ioTrack.face.trackId << ") is detected";
    }
    else if (iStatus == TrackStatus::Tracked)
    {
      LOG_EVERY_N(10, INFO) << "User(" << ioTrack.face.trackId << ") is tracked";
    }
    else
    {
      LOG(INFO) << "User(" << ioTrack.face.trackId << ") is inactivated";
    }
  }

  std::size_t FaceTracker::GetActiveTrackCount() const
  {
    std::size_t count = 0U;

    for (const auto& track : mTracks)
    {
      if (track.face.status != TrackStatus::Inactive)
        count++;
    }

    return count;
  }
}
