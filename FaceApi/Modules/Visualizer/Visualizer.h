#pragma once

#include "Framework/Imaging/Geometry.h"
#include "Framework/ErrorCode.h"
#include "Framework/Port.hpp"
#include "Framework/Stopwatch.h"

#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"

#include <memory>
#include <opencv2/core/core.hpp>
#include <limits>
#include <vector>

namespace face
{
  class Visualizer : public fw::Module,
                     public fw::Port<std::shared_ptr<ImageMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<ActiveUsersMessage>)>
  {
  public:

    Visualizer() = default;

    virtual ~Visualizer() = default;

    std::shared_ptr<ImageMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<ActiveUsersMessage> iUsers) override;

    void Clear() override
    {
      mMinRuntimeMs = (std::numeric_limits<double>::max)();
      mMaxRuntimeMs = (std::numeric_limits<double>::lowest)();
    }

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void DrawShapeModel(const User& iUser, cv::Mat& oImage) const;

    void DrawUserData(const User& iUser, cv::Mat& oImage) const;

    void DrawAxes(const User& iUser, cv::Mat& oImage) const;

    void DrawBoundingBox(const User& iUser, cv::Mat& oImage, int iSegmentWidth = 5, int iThickness = 1) const;

    void DrawGeneral(std::shared_ptr<ImageMessage> iImage, cv::Mat& oImage);

    void CreateShapeColorMap(const fw::VectorPt3D& iShape3D, cv::Mat& oColorMap) const;

    std::vector<cv::Scalar> mColorsOfAxes;

    double mMinRuntimeMs = (std::numeric_limits<double>::max)();
    double mMaxRuntimeMs = (std::numeric_limits<double>::lowest)();
  };
}
