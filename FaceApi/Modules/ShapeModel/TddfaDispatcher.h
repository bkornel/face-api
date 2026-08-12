#pragma once

#include "Framework/ErrorCode.h"
#include "Framework/OneEuroFilter.h"
#include "Framework/TimeExtensions.h"
#include "Modules/ShapeModel/IShapeFitter.h"

#include <opencv2/dnn.hpp>

#include <map>
#include <mutex>
#include <vector>

namespace face
{
  /// @brief The 3DDFA_V2 fitter: a MobileNet regressing 62 3DMM parameters from a 120x120
  /// face crop, decoded to the 68 landmark subset of the Basel Face Model and mapped onto
  /// the 66-point layout the rest of the pipeline speaks. Trained on large poses, so it
  /// keeps working when the head is turned far from the camera.
  ///
  /// The network is shared and cv::dnn forward() is not reentrant, so inference is
  /// serialized; everything around it runs per track. A One Euro filter per landmark takes
  /// the per-frame regression jitter out.
  class TddfaDispatcher : public IShapeFitter
  {
  public:
    TddfaDispatcher() = default;

    virtual ~TddfaDispatcher() = default;

    fw::ErrorCode Initialize(const cv::FileNode& iSettings) override;

    void BeginFrame(const std::vector<TrackedFace>& iTracks) override;

    bool Fit(const TrackedFace& iTrack, const cv::Mat& iFrameGray, const cv::Mat& iFrameBGR, ShapeDescriptor& oShape) override;

    void Clear() override;

  private:
    static constexpr int cInputSize = 120;
    static constexpr int cParams = 62;
    static constexpr int cLandmarks = 68;

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

    std::map<int, TrackState> mStates;

    bool mSmoothing = true;
    double mSmoothMinCutoff = 1.0;
    double mSmoothBeta = 0.5;
  };
}
