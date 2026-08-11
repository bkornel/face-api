#include "Framework/Imaging/Procrustes.h"

namespace fw
{
  void recenter_shapes(ShapeVector& ioShapes)
  {
    for (auto& shape : ioShapes)
    {
      const cv::Scalar& mean = cv::mean(shape);
      shape = shape - cv::Mat(shape.size(), shape.type(), mean);
    }
  }

  void normalize_shapes(ShapeVector& ioShapes)
  {
    for (auto& shape : ioShapes)
    {
      const double norm = cv::norm(shape);
      if (norm > 0.0) shape = shape / norm;
    }
  }

  void align_shapes(ShapeVector& ioShapes, cv::Mat& ioMeanShape)
  {
    cv::Mat w, u, vt;

    for (auto& shape : ioShapes)
    {
      cv::SVDecomp(ioMeanShape.reshape(1).t() * shape.reshape(1), w, u, vt);
      shape = (shape.reshape(1) * vt.t()) * u.t();
    }
  }

  void generalized_procrustes(ShapeVector& ioShapes, cv::Mat& ioMeanShape, int iMaxIterations, double iEpsilon)
  {
    if (ioShapes.empty()) return;

    int iteration = 0;

    while (true)
    {
      recenter_shapes(ioShapes);
      normalize_shapes(ioShapes);
      align_shapes(ioShapes, ioMeanShape);

      // Find a new mean shape: std::accumulate returns the sum, discarding it left zeros.
      cv::Mat newMeanShape = cv::Mat::zeros(ioMeanShape.size(), ioMeanShape.type());
      for (const auto& shape : ioShapes)
        newMeanShape += shape;

      newMeanShape = newMeanShape / static_cast<double>(ioShapes.size());

      const double meanNorm = cv::norm(newMeanShape);
      if (meanNorm > 0.0)
        newMeanShape = newMeanShape / meanNorm;

      // Until the mean stops moving, or the iteration budget runs out
      const double difference = cv::norm(newMeanShape, ioMeanShape);
      if (iteration++ > iMaxIterations || difference <= iEpsilon)
        break;

      ioMeanShape = newMeanShape;
    }
  }
}
