// Regenerates the mShape3D table of FaceModel from the mean landmark shape of the Basel
// model, which is the same geometry the shape model network regresses. Prints the C++
// initializer to standard output, ready to paste into FaceModel.cpp.
//
//   GenerateFaceModel <path to u_base.bin>
//
// Build it against OpenCV core, e.g. from a developer prompt:
//   cl /EHsc /MD /O2 /I <opencv>\include GenerateFaceModel.cpp
//      /link /LIBPATH:<opencv>\x64\vc16\lib opencv_core452.lib
#include <opencv2/core.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace
{
  constexpr int cLandmarks = 68;

  // The nose tip, which the model puts at the origin
  constexpr int cOriginLandmark = 30;

  // The size the head is scaled to, as the root mean square distance of the landmarks from
  // their centroid. Millimetres: it is what the previous, hand-written table measured, and
  // keeping it means the position a pose estimate reports stays in the same units.
  constexpr double cRmsRadiusMm = 54.56;

  double RmsRadius(const std::vector<cv::Point3d>& iPoints)
  {
    cv::Point3d centroid(0.0, 0.0, 0.0);
    for (const auto& pt : iPoints) centroid += pt;
    centroid /= static_cast<double>(iPoints.size());

    double sum = 0.0;
    for (const auto& pt : iPoints) sum += (pt - centroid).dot(pt - centroid);

    return std::sqrt(sum / iPoints.size());
  }
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::printf("usage: GenerateFaceModel <path to u_base.bin>\n");
    return 1;
  }

  std::vector<float> uBase(cLandmarks * 3);
  {
    std::ifstream file(argv[1], std::ios::binary);
    file.read(reinterpret_cast<char*>(uBase.data()), static_cast<std::streamsize>(uBase.size() * sizeof(float)));

    if (!file)
    {
      std::printf("cannot read %s\n", argv[1]);
      return 1;
    }
  }

  std::vector<cv::Point3d> shape;
  shape.reserve(cLandmarks);
  for (int i = 0; i < cLandmarks; ++i)
    shape.emplace_back(uBase[i * 3], uBase[i * 3 + 1], uBase[i * 3 + 2]);

  // The morphable model is y-up and looks down -z; this project works in image orientation
  for (auto& pt : shape) { pt.y = -pt.y; pt.z = -pt.z; }

  const double scale = cRmsRadiusMm / RmsRadius(shape);
  for (auto& pt : shape) pt *= scale;

  const cv::Point3d origin = shape[cOriginLandmark];
  for (auto& pt : shape) pt -= origin;

  std::printf("    mShape3D = {\n");
  for (int i = 0; i < cLandmarks; ++i)
  {
    const char* section = "";
    if (i == 0) section = "  // Contour";
    else if (i == 17) section = "  // Eyebrows";
    else if (i == 27) section = "  // Nose";
    else if (i == cOriginLandmark) section = "  // Origin: the nose tip";
    else if (i == 36) section = "  // Eyes";
    else if (i == 48) section = "  // Outer lip";
    else if (i == 60) section = "  // Inner lip";

    std::printf("      { %7.2f, %7.2f, %7.2f }%s%s\n",
                shape[i].x, shape[i].y, shape[i].z, i == cLandmarks - 1 ? "" : ",", section);
  }
  std::printf("    };\n");

  std::fprintf(stderr, "scaled by %.6g, face width %.1f mm\n", scale, shape[16].x - shape[0].x);
  return 0;
}
