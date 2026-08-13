#include "Framework/Settings.h"
#include "Framework/ErrorCode.h"
#include "Modules/ShapeModel/TddfaDispatcher.h"

#include "Configuration.h"
#include "Framework/Text.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>

namespace face
{
  fw::ErrorCode TddfaDispatcher::Initialize(const cv::FileNode& iSettings)
  {
    std::string modelDir = "3ddfa/";

    if (!iSettings.empty())
    {
      std::string value;

      if (fw::get_value(iSettings, "modelDir", value))
        modelDir = value;

      if (fw::get_value(iSettings, "smoothing", value))
        mSmoothing = fw::str::convert_to_boolean(value);

      if (fw::get_value(iSettings, "smoothMinCutoff", value))
        mSmoothMinCutoff = fw::str::convert_to_number<double>(value);

      if (fw::get_value(iSettings, "smoothBeta", value))
        mSmoothBeta = fw::str::convert_to_number<double>(value);
    }

    const std::string path = Configuration::GetInstance().GetDirectories().shapeModel + modelDir;

    try
    {
      mNet = cv::dnn::readNetFromONNX(path + "mb1_120x120.onnx");
    }
    catch (const cv::Exception& iException)
    {
      LOG(ERROR) << "Could not load the 3DDFA model from " << path << ": " << iException.what();
      return fw::ErrorCode::NotFound;
    }

    if (!LoadBin(path + "param_mean.bin", cParams, mParamMean) ||
        !LoadBin(path + "param_std.bin", cParams, mParamStd) ||
        !LoadBin(path + "u_base.bin", cLandmarks * 3, mUBase) ||
        !LoadBin(path + "w_shp_base.bin", cLandmarks * 3 * 40, mWShp) ||
        !LoadBin(path + "w_exp_base.bin", cLandmarks * 3 * 10, mWExp))
    {
      LOG(ERROR) << "Could not load the 3DDFA basis data from " << path;
      return fw::ErrorCode::NotFound;
    }

    mModelScale = MeasureModelScale();

    return fw::ErrorCode::OK;
  }

  double TddfaDispatcher::MeasureModelScale() const
  {
    // The root mean square distance of the mean shape's landmarks from their centroid, and
    // the size FaceModel's table was scaled to. Doing the same arithmetic here is what makes
    // a fitted shape and the canonical model comparable rather than merely similar.
    constexpr double cRmsRadiusMm = 54.56;

    cv::Point3d centroid(0.0, 0.0, 0.0);

    for (int i = 0; i < cLandmarks; ++i)
    {
      centroid += cv::Point3d(mUBase[i * 3], mUBase[i * 3 + 1], mUBase[i * 3 + 2]);
    }

    centroid /= static_cast<double>(cLandmarks);

    double sum = 0.0;

    for (int i = 0; i < cLandmarks; ++i)
    {
      const cv::Point3d offset = cv::Point3d(mUBase[i * 3], mUBase[i * 3 + 1], mUBase[i * 3 + 2]) - centroid;
      sum += offset.dot(offset);
    }

    const double rms = std::sqrt(sum / cLandmarks);

    return rms > 1e-9 ? cRmsRadiusMm / rms : 1.0;
  }

  bool TddfaDispatcher::LoadBin(const std::string& iPath, std::size_t iCount, std::vector<float>& oData) const
  {
    std::ifstream file(iPath, std::ios::binary);

    oData.resize(iCount);
    file.read(reinterpret_cast<char*>(oData.data()), static_cast<std::streamsize>(iCount * sizeof(float)));

    return static_cast<bool>(file);
  }

  void TddfaDispatcher::BeginFrame(const std::vector<TrackedFace>& iTracks)
  {
    std::erase_if(mStates, [&iTracks](const std::map<int, TrackState>::value_type& iEntry) {
      return std::none_of(iTracks.begin(), iTracks.end(), [&iEntry](const TrackedFace& iTrack) {
        return iTrack.trackId == iEntry.first;
      });
    });

    // Every track gets its entry here, so the concurrent fits below only ever read the map
    for (const auto& track : iTracks)
    {
      if (mStates.find(track.trackId) == mStates.end())
        mStates.emplace(track.trackId, TrackState{});
    }
  }

  void TddfaDispatcher::Clear()
  {
    mStates.clear();
  }

  cv::Rect2d TddfaDispatcher::RoiFromBbox(const cv::Rect& iBbox) const
  {
    // parse_roi_box_from_bbox of the reference implementation
    const double oldSize = (iBbox.width + iBbox.height) / 2.0;
    const double centerX = iBbox.x + iBbox.width / 2.0;
    const double centerY = iBbox.y + iBbox.height / 2.0 + oldSize * 0.14;
    const double size = std::floor(oldSize * 1.58);

    return { centerX - size / 2.0, centerY - size / 2.0, size, size };
  }

  cv::Rect2d TddfaDispatcher::RoiFromShape(const std::vector<cv::Point2d>& iShape) const
  {
    // parse_roi_box_from_landmark of the reference implementation
    double minX = iShape[0].x, maxX = iShape[0].x;
    double minY = iShape[0].y, maxY = iShape[0].y;

    for (const auto& pt : iShape)
    {
      minX = (std::min)(minX, pt.x);
      maxX = (std::max)(maxX, pt.x);
      minY = (std::min)(minY, pt.y);
      maxY = (std::max)(maxY, pt.y);
    }

    const double radius = (std::max)(maxX - minX, maxY - minY) / 2.0;
    const double centerX = (minX + maxX) / 2.0;
    const double centerY = (minY + maxY) / 2.0;

    // The square around the landmarks, blown up to its diagonal
    const double length = radius * 2.0 * std::numbers::sqrt2;

    return { centerX - length / 2.0, centerY - length / 2.0, length, length };
  }

  cv::Mat TddfaDispatcher::CropRoi(const cv::Mat& iFrameBGR, const cv::Rect2d& iRoi) const
  {
    const cv::Rect roi(cvRound(iRoi.x), cvRound(iRoi.y), cvRound(iRoi.width), cvRound(iRoi.height));

    cv::Mat crop = cv::Mat::zeros(roi.height, roi.width, CV_8UC3);

    const cv::Rect source = roi & cv::Rect(0, 0, iFrameBGR.cols, iFrameBGR.rows);
    if (source.area() > 0)
    {
      const cv::Rect destination(source.x - roi.x, source.y - roi.y, source.width, source.height);
      iFrameBGR(source).copyTo(crop(destination));
    }

    return crop;
  }

  bool TddfaDispatcher::Fit(const TrackedFace& iTrack, const cv::Mat& iFrameBGR, ShapeDescriptor& oShape)
  {
    auto it = mStates.find(iTrack.trackId);
    if (it == mStates.end()) return false;

    TrackState& state = it->second;

    // A fresh detection re-anchors the crop; otherwise it follows the previous landmarks
    const bool fromShape = state.hasShape && iTrack.status != TrackStatus::Detected;
    const cv::Rect2d roi = fromShape ? RoiFromShape(state.lastShape) : RoiFromBbox(iTrack.faceRect);

    if (roi.width < 1.0 || roi.height < 1.0) return false;

    cv::Mat input;
    cv::resize(CropRoi(iFrameBGR, roi), input, { cInputSize, cInputSize });

    // (x - 127.5) / 128, BGR, NCHW
    const cv::Mat blob = cv::dnn::blobFromImage(input, 1.0 / 128.0, {}, cv::Scalar(127.5, 127.5, 127.5), false);

    std::vector<double> param(cParams);
    {
      // forward() is not reentrant and hands out a view into the network, hence the copy
      std::lock_guard<std::mutex> lock(mNetMutex);

      mNet.setInput(blob);
      const cv::Mat out = mNet.forward();

      for (int i = 0; i < cParams; ++i)
        param[i] = out.ptr<float>()[i] * mParamStd[i] + mParamMean[i];
    }

    // verts = u + w_shp * alpha_shp + w_exp * alpha_exp, laid out x,y,z per landmark
    std::vector<cv::Point2d> shape68(cLandmarks);
    std::vector<cv::Point3d> shape3D(cLandmarks);

    // The first twelve parameters are the pose: a 3x4 similarity whose first two rows are
    // what projects a landmark to the image. Only those two are trustworthy - the third is
    // regressed but never used by the projection, so nothing trains it - and two rows are
    // enough: normalise them and the third axis is their cross product.
    //
    // Image y grows downwards while the model's grows upwards, so the second row flips; and
    // the shape is reported in the same flipped frame, so the rotation is conjugated by that
    // flip to act on it.
    {
      const cv::Vec3d right(param[0], param[1], param[2]);
      const cv::Vec3d down(-param[4], -param[5], -param[6]);

      const double rightLength = cv::norm(right);
      const double downLength = cv::norm(down);

      if (rightLength > 1e-9 && downLength > 1e-9)
      {
        const cv::Vec3d r1 = right / rightLength;
        const cv::Vec3d r2 = down / downLength;
        const cv::Vec3d r3 = r1.cross(r2);

        const cv::Matx33d toCamera(r1[0], r1[1], r1[2],
                                   r2[0], r2[1], r2[2],
                                   r3[0], r3[1], r3[2]);

        const cv::Matx33d flip(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, -1.0);

        oShape.rotation = toCamera * flip;
        oShape.hasRotation = true;
      }
    }

    const double scaleX = roi.width / cInputSize;
    const double scaleY = roi.height / cInputSize;

    for (int i = 0; i < cLandmarks; ++i)
    {
      double vertex[3];

      for (int c = 0; c < 3; ++c)
      {
        const int k = i * 3 + c;
        double v = mUBase[k];

        for (int j = 0; j < 40; ++j) v += mWShp[k * 40 + j] * param[12 + j];
        for (int j = 0; j < 10; ++j) v += mWExp[k * 10 + j] * param[52 + j];

        vertex[c] = v;
      }

      // The shape before the pose is applied: this is the face itself, and the projection
      // below is what turns it into the 2-D landmarks. It used to be discarded here, which
      // left the pose module's rigidly moved canonical model as the pipeline's only 3-D
      // shape - a shape with no expression in it whatsoever.
      shape3D[i] = { vertex[0] * mModelScale, -vertex[1] * mModelScale, -vertex[2] * mModelScale };

      // pts = R * verts + offset, then the similar transform back to frame coordinates,
      // which flips y: the morphable model lives in a y-up space
      const double x = param[0] * vertex[0] + param[1] * vertex[1] + param[2] * vertex[2] + param[3];
      const double y = param[4] * vertex[0] + param[5] * vertex[1] + param[6] * vertex[2] + param[7];

      shape68[i] = {
        (x - 1.0) * scaleX + roi.x,
        (cInputSize - y) * scaleY + roi.y
      };

      if (cvIsNaN(shape68[i].x) || cvIsInf(shape68[i].x) || cvIsNaN(shape68[i].y) || cvIsInf(shape68[i].y))
      {
        state.hasShape = false;
        return false;
      }
    }

    // The canonical model puts the nose tip at the origin, and a shape that is going to be
    // compared with it has to agree
    const cv::Point3d origin = shape3D[cOriginLandmark];
    for (auto& point : shape3D) point -= origin;

    // The raw shape drives the next frame's crop; smoothing only shapes what is reported
    state.lastShape = shape68;
    state.hasShape = true;

    if (mSmoothing)
    {
      if (state.filters.empty())
      {
        // Two channels per landmark for the 2-D shape and three for the 3-D one
        state.filters.assign(cLandmarks * 5, fw::OneEuroFilter(mSmoothMinCutoff, mSmoothBeta));
        state.lastTimestamp = iTrack.lastUpdateTs;
      }

      double dt = fw::elapsed(state.lastTimestamp, iTrack.lastUpdateTs).count() / 1000.0;
      if (dt <= 0.0) dt = 1.0 / 30.0;

      state.lastTimestamp = iTrack.lastUpdateTs;

      const int firstOf3D = cLandmarks * 2;

      for (int i = 0; i < cLandmarks; ++i)
      {
        shape68[i].x = state.filters[i * 2].Filter(shape68[i].x, dt);
        shape68[i].y = state.filters[i * 2 + 1].Filter(shape68[i].y, dt);

        shape3D[i].x = state.filters[firstOf3D + i * 3].Filter(shape3D[i].x, dt);
        shape3D[i].y = state.filters[firstOf3D + i * 3 + 1].Filter(shape3D[i].y, dt);
        shape3D[i].z = state.filters[firstOf3D + i * 3 + 2].Filter(shape3D[i].z, dt);
      }
    }

    cv::Point2d minPt = shape68[0];
    cv::Point2d maxPt = shape68[0];

    for (const auto& pt : shape68)
    {
      minPt.x = (std::min)(minPt.x, pt.x);
      minPt.y = (std::min)(minPt.y, pt.y);
      maxPt.x = (std::max)(maxPt.x, pt.x);
      maxPt.y = (std::max)(maxPt.y, pt.y);
    }

    // Clipped rather than rejected: this fitter keeps a turned face, and a turned face may
    // legitimately reach past the frame
    const cv::Rect fittedRect = cv::Rect(minPt, maxPt) & cv::Rect(0, 0, iFrameBGR.cols, iFrameBGR.rows);
    if (fittedRect.area() <= 0)
    {
      state.hasShape = false;
      return false;
    }

    oShape.trackId = iTrack.trackId;
    oShape.faceRect = fittedRect;
    oShape.shape2D = std::move(shape68);
    oShape.shape3D = std::move(shape3D);

    return true;
  }
}
