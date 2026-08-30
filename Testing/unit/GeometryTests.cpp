// The accuracy net. Nothing here compares against a recorded output: every expectation is
// something the arithmetic must satisfy - a pose that reprojects onto the landmarks it was
// solved from, a shape that normalises to the same thing however it is placed, an expression
// measure that moves the way the face moved. A refactor that quietly changes the geometry
// fails these; a change of camera or model does not.
#include "TestSupport.h"

#include "Framework/Imaging/Geometry.h"
#include "Framework/Imaging/Projection.h"
#include "Framework/MathExtensions.h"
#include "Model/FaceModel.h"
#include "Model/ShapeMetrics.h"
#include "Modules/HeadPose/PoseEstimationDispatcher.h"
#include "Modules/ShapeNorm/ShapeNormDispatcher.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>

#include <cmath>
#include <vector>

namespace
{
  constexpr int cFrameWidth = 640;
  constexpr int cFrameHeight = 480;

  /// @brief The canonical model placed at a known pose and projected, which is a face the
  /// answer is known for
  face::ShapeDescriptor SynthesiseShape(const cv::Vec3d& iRvec, const cv::Vec3d& iTvec)
  {
    const fw::VectorPt3D& model = face::FaceModel::GetInstance().GetShape3D();
    const cv::Mat cameraMatrix = fw::get_camera_matrix({ cFrameWidth, cFrameHeight });

    face::ShapeDescriptor shape;
    shape.trackId = 0;
    shape.shape3D = model;

    cv::projectPoints(model, cv::Mat(iRvec), cv::Mat(iTvec), cameraMatrix, cv::noArray(), shape.shape2D);

    cv::Mat rotation;
    cv::Rodrigues(cv::Mat(iRvec), rotation);

    // What a shape model would have regressed for this pose
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) shape.rotation(r, c) = rotation.at<double>(r, c);

    shape.hasRotation = true;

    return shape;
  }

  double ReprojectionError(const face::PoseDescriptor& iPose, const fw::VectorPt2D& iExpected)
  {
    fw::VectorPt2D projected;
    cv::projectPoints(face::FaceModel::GetInstance().GetShape3D(), iPose.rvec, iPose.tvec,
                      iPose.cameraMatrix, cv::noArray(), projected);

    double worst = 0.0;

    for (std::size_t i = 0U; i < projected.size() && i < iExpected.size(); ++i)
      worst = (std::max)(worst, cv::norm(projected[i] - iExpected[i]));

    return worst;
  }

  /// @param iPoseFromShapeModel Whether the rotation is taken from the shape model or solved
  face::PoseDescriptor Estimate(const face::ShapeDescriptor& iShape, bool iPoseFromShapeModel)
  {
    const std::string json =
      std::string("{ \"headPose\": { \"poseFromShapeModel\": \"") +
      (iPoseFromShapeModel ? "TRUE" : "FALSE") + "\" } }";

    cv::FileStorage storage(json, cv::FileStorage::READ | cv::FileStorage::MEMORY |
                                    cv::FileStorage::FORMAT_JSON);

    face::PoseEstimationDispatcher dispatcher;
    dispatcher.Initialize(storage["headPose"]);

    face::PoseDescriptor pose;
    dispatcher.Estimate(iShape, fw::get_camera_matrix({ cFrameWidth, cFrameHeight }), pose);

    return pose;
  }

  void APoseIsRecoveredFromTheLandmarksItProduced()
  {
    const cv::Vec3d rvec(fw::deg_to_rad(-8.0), fw::deg_to_rad(14.0), fw::deg_to_rad(5.0));
    const cv::Vec3d tvec(23.0, -17.0, 620.0);

    const face::ShapeDescriptor shape = SynthesiseShape(rvec, tvec);

    for (int pass = 0; pass < 2; ++pass)
    {
      const bool fromShapeModel = (pass == 0);
      const std::string what = fromShapeModel ? "rotation from the shape model" : "pose solved from the landmarks";

      const face::PoseDescriptor pose = Estimate(shape, fromShapeModel);

      test::Check(!pose.rvec.empty() && !pose.tvec.empty(), what + ": a pose comes back");

      test::Near(pose.tvec.at<double>(0), tvec[0], 1.0, what + ": x is recovered");
      test::Near(pose.tvec.at<double>(1), tvec[1], 1.0, what + ": y is recovered");
      test::Near(pose.tvec.at<double>(2), tvec[2], 2.0, what + ": distance is recovered");

      // The measure that does not depend on any convention: the recovered pose has to put
      // the model back where the landmarks are
      test::Near(ReprojectionError(pose, shape.shape2D), 0.0, 1.5, what + ": reprojects onto the landmarks");
    }
  }

  void TheAnglesAreRollPitchYawInThatOrder()
  {
    // One axis at a time, so which slot moved says what the convention is. Everything that
    // reads rpy - the overlay, the head view, the C interface - depends on this order.
    const struct
    {
      const char* name;
      cv::Vec3d rvec;
      int slot;
    } cases[] = {
      { "roll",  { 0.0, 0.0, fw::deg_to_rad(12.0) }, 0 },
      { "pitch", { fw::deg_to_rad(12.0), 0.0, 0.0 }, 1 },
      { "yaw",   { 0.0, fw::deg_to_rad(12.0), 0.0 }, 2 }
    };

    for (const auto& item : cases)
    {
      const face::PoseDescriptor pose = Estimate(SynthesiseShape(item.rvec, { 0.0, 0.0, 600.0 }), true);

      test::Near(std::abs(fw::rad_to_deg(pose.rpy[item.slot])), 12.0, 1.5,
                 std::string("a pure ") + item.name + " lands in rpy[" + std::to_string(item.slot) + "]");

      for (int other = 0; other < 3; ++other)
      {
        if (other == item.slot) continue;

        test::Near(fw::rad_to_deg(pose.rpy[other]), 0.0, 1.5,
                   std::string("and leaves rpy[") + std::to_string(other) + "] alone");
      }
    }
  }

  void TheFaceBoxIsPlacedWithTheHead()
  {
    const cv::Vec3d tvec(0.0, 0.0, 500.0);
    const face::PoseDescriptor pose = Estimate(SynthesiseShape({ 0.0, 0.0, 0.0 }, tvec), true);

    test::Check(pose.faceBox.size() == 8U, "the face box has eight corners");
    test::Check(pose.shape3D.size() == face::FaceModel::GetInstance().GetShape3D().size(),
                "the 3-D shape is reported for every landmark");

    // The shape is moved into the camera, so it has to sit around where the head is
    cv::Point3d centroid(0.0, 0.0, 0.0);
    for (const auto& point : pose.shape3D) centroid += point;
    centroid /= static_cast<double>(pose.shape3D.size());

    test::Near(centroid.z, tvec[2], 60.0, "the placed shape sits at the head's distance");
  }

  void NormalisingRemovesPositionScaleAndRotation()
  {
    face::ShapeNormDispatcher dispatcher;
    dispatcher.Initialize(cv::FileNode());

    const fw::VectorPt2D& reference = face::FaceModel::GetInstance().GetFrontalShape2D();

    // The same shape, moved across the frame, made half the size and turned by 20 degrees.
    // Normalising is what makes two faces comparable, so both have to come back the same.
    const double angle = fw::deg_to_rad(20.0);
    const double scale = 0.5;

    fw::VectorPt2D placed;
    placed.reserve(reference.size());

    for (const auto& point : reference)
    {
      const double x = point.x * scale;
      const double y = point.y * scale;

      placed.emplace_back(x * std::cos(angle) - y * std::sin(angle) + 320.0,
                          x * std::sin(angle) + y * std::cos(angle) + 240.0);
    }

    const fw::VectorPt2D normalisedReference = dispatcher.Normalize2D(reference);
    const fw::VectorPt2D normalisedPlaced = dispatcher.Normalize2D(placed);

    test::Check(normalisedReference.size() == reference.size(), "the normalised shape keeps every point");

    double worst = 0.0;
    for (std::size_t i = 0U; i < normalisedReference.size(); ++i)
      worst = (std::max)(worst, cv::norm(normalisedReference[i] - normalisedPlaced[i]));

    test::Near(worst, 0.0, 1e-3, "a moved, scaled and turned shape normalises onto the same one");

    // Centred and of unit size, which is what the mesh driving it expects
    cv::Point2d centroid(0.0, 0.0);
    for (const auto& point : normalisedPlaced) centroid += point;
    centroid /= static_cast<double>(normalisedPlaced.size());

    test::Near(cv::norm(centroid), 0.0, 1e-6, "the normalised shape is centred on the origin");
    test::Near(cv::norm(cv::Mat(normalisedPlaced)), 1.0, 1e-6, "and scaled to unit norm");
  }

  void TheExpressionMeasuresFollowTheFace()
  {
    const fw::VectorPt2D& neutral = face::FaceModel::GetInstance().GetFrontalShape2D();

    const face::ExpressionMetrics rest = face::measure_expression(neutral);

    const double values[] = { rest.openMouth, rest.openEyeLeft, rest.openEyeRight, rest.browRaise, rest.smile };
    bool inRange = true;

    for (const double value : values)
      inRange = inRange && (value >= 0.0) && (value <= 1.0);

    test::Check(inRange, "every measure of the neutral face is inside [0, 1]");
    test::Check(rest.openMouth < 0.35, "the neutral face is not reported as open-mouthed");

    // Open the jaw: the inner lip contour is what the measure reads
    fw::VectorPt2D open = neutral;
    const int innerFirst = face::index_of(face::Landmark::kMouth12);
    const double interocular = face::interocular_distance(neutral);

    for (int i = 1; i <= 3; ++i) open[innerFirst + i].y -= interocular * 0.10;
    for (int i = 5; i <= 7; ++i) open[innerFirst + i].y += interocular * 0.10;

    const face::ExpressionMetrics opened = face::measure_expression(open);

    test::Check(opened.openMouth > rest.openMouth + 0.2,
                "opening the inner lip raises openMouth (" + std::to_string(rest.openMouth) +
                  " -> " + std::to_string(opened.openMouth) + ")");

    // Close the eyes: the lids come together, the aspect ratio collapses
    fw::VectorPt2D shut = neutral;
    const int leftEye = face::index_of(face::Landmark::kLeftEye0);

    for (int i = 0; i < 6; ++i)
    {
      const cv::Point2d& corner = neutral[leftEye];
      shut[leftEye + i].y = corner.y;
    }

    const face::ExpressionMetrics blinked = face::measure_expression(shut);

    test::Check(blinked.openEyeLeft < rest.openEyeLeft,
                "flattening an eye lowers its openness (" + std::to_string(rest.openEyeLeft) +
                  " -> " + std::to_string(blinked.openEyeLeft) + ")");

    test::Check(face::measure_expression(fw::VectorPt2D(10U)).openMouth == 0.0,
                "a shape that is not the 68-point layout reports nothing");
  }

  void DepthWeightsFadeTheFarSide()
  {
    fw::VectorPt3D shape = { { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 50.0 }, { 2.0, 0.0, 100.0 } };

    const std::vector<double> weights = fw::depth_weights(shape);

    test::Check(weights.size() == shape.size(), "one weight per point");
    test::Near(weights.front(), 1.0, 1e-9, "the nearest point is at full strength");
    test::Check(weights.back() < weights.front(), "the far point is faded");

    // A flat shape has no front and no back, so nothing may be faded
    const std::vector<double> flat = fw::depth_weights({ { 0.0, 0.0, 7.0 }, { 1.0, 0.0, 7.0 } });
    test::Check(flat.size() == 2U && flat[0] == 1.0 && flat[1] == 1.0, "a flat shape is not faded at all");
  }
}

void RunGeometryTests()
{
  test::Section("pose, shape and expression accuracy");

  APoseIsRecoveredFromTheLandmarksItProduced();
  TheAnglesAreRollPitchYawInThatOrder();
  TheFaceBoxIsPlacedWithTheHead();
  NormalisingRemovesPositionScaleAndRotation();
  TheExpressionMeasuresFollowTheFace();
  DepthWeightsFadeTheFarSide();
}
