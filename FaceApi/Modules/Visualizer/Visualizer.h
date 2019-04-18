#pragma once

#include "Messages/ImageMessage.h"
#include "Messages/ActiveUsersMessage.h"
#include "Modules/General/ModuleWithPort.hpp"

#include <opencv2/core/core.hpp>
#include <vector>

namespace face
{
  class Visualizer :
    public ModuleWithPort<ImageMessage::Shared(ImageMessage::Shared, ActiveUsersMessage::Shared)>
  {
  public:
    Visualizer() = default;

    virtual ~Visualizer() = default;

    ImageMessage::Shared Main(ImageMessage::Shared iImage, ActiveUsersMessage::Shared iUsers) override;

  private:
    fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

    void DrawShapeModel(const User& iUser, cv::Mat& oImage) const;

    void DrawUserData(const User& iUser, cv::Mat& oImage) const;

    void DrawAxes(const User& iUser, cv::Mat& oImage) const;

    void DrawBoundingBox(const User& iUser, cv::Mat& oImage, int iSegmentWidth = 5, int iThickness = 1) const;

    void DrawGeneral(ImageMessage::Shared iImage, cv::Mat& oImage) const;

    void CreateShapeColorMap(const fw::ocv::VectorPt3D& iShape3D, cv::Mat& oColorMap) const;

    std::vector<cv::Scalar> mColorsOfAxes;
  };
}
