#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/OneEuroFilter.h"
#include "Framework/TimeExtensions.h"
#include "Messages/ShapeMessage.h"
#include "User/TrackedFace.h"

#include <opencv2/dnn.hpp>

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace face
{
  /// @brief The 3DDFA_V2 fitter: a MobileNet regressing 62 3DMM parameters from a 120x120
  /// face crop, decoded to the 68 landmarks of the Basel Face Model, which is the layout the
  /// rest of the pipeline speaks. Trained on large poses, so it keeps working when the head
  /// is turned far from the camera.
  ///
  /// The network is shared and cv::dnn forward() is not reentrant, so inference is
  /// serialized; everything around it runs per track. A One Euro filter per landmark takes
  /// the per-frame regression jitter out.
  class TddfaDispatcher
  {
  public:
    TddfaDispatcher() = default;

    ~TddfaDispatcher() = default;

    /// @param iWorkingDirectory What the modelDir setting is relative to
    fw::ErrorCode Initialize(const cv::FileNode& iSettings, const std::string& iWorkingDirectory);

    void BeginFrame(const std::vector<TrackedFace>& iTracks);

    bool Fit(const TrackedFace& iTrack, const cv::Mat& iFrameBGR, ShapeDescriptor& oShape);

    void Clear();

  private:
    static constexpr int cInputSize = 120;
    static constexpr int cParams = 62;
    static constexpr int cLandmarks = 68;

    /// @brief The nose tip, which the canonical model puts at the origin
    static constexpr int cOriginLandmark = 30;

    /// @brief What the morphable model's own units have to be multiplied by to become the
    /// canonical model's. Derived from the mean shape exactly as Testing/tools did when it
    /// generated FaceModel's table, so a fitted average face reproduces that table.
    double MeasureModelScale() const;

    struct TrackState
    {
      // The crop of the next frame follows the landmarks of the previous one
      std::vector<cv::Point2d> lastShape;
      bool hasShape = false;

      std::vector<fw::OneEuroFilter> filters;
      fw::Timestamp lastTimestamp;
    };

    bool LoadBin(const std::string& iPath, std::size_t iCount, std::vector<float>& oData) const;

    cv::Rect2d RoiFromBbox(const cv::Rect& iBbox) const;

    cv::Rect2d RoiFromShape(const std::vector<cv::Point2d>& iShape) const;

    cv::Mat CropRoi(const cv::Mat& iFrameBGR, const cv::Rect2d& iRoi) const;

    std::mutex mNetMutex;
    cv::dnn::Net mNet;

    std::vector<float> mParamMean;
    std::vector<float> mParamStd;
    std::vector<float> mUBase;
    std::vector<float> mWShp;
    std::vector<float> mWExp;

    double mModelScale = 1.0;

    std::map<int, TrackState> mStates;

    bool mSmoothing = true;
    double mSmoothMinCutoff = 1.0;
    double mSmoothBeta = 0.5;
  };
}
