#pragma once

#include <opencv2/core.hpp>

#include <vector>

namespace fw
{
  /// @brief Generalized Procrustes analysis: aligns a set of shapes to each other by removing
  /// the differences in position, scale and rotation between them, leaving only the difference
  /// in form. A shape is one column matrix of coordinates, the same layout for every entry.
  using ShapeVector = std::vector<cv::Mat>;

  /// @brief Moves every shape so that its centroid is at the origin
  void recenter_shapes(ShapeVector& ioShapes);

  /// @brief Scales every shape to unit norm
  void normalize_shapes(ShapeVector& ioShapes);

  /// @brief Rotates every shape onto ioMeanShape, through the SVD of their cross covariance
  void align_shapes(ShapeVector& ioShapes, cv::Mat& ioMeanShape);

  /// @brief Recentres, normalizes and aligns repeatedly, refining ioMeanShape until it moves
  /// by less than iEpsilon or iMaxIterations is reached.
  /// @param ioMeanShape in: the shape to start from, out: the converged mean
  void generalized_procrustes(ShapeVector& ioShapes, cv::Mat& ioMeanShape, int iMaxIterations, double iEpsilon);
}
