// Renders the synthetic face videos under Testing/media: a haar-friendly cartoon face
// (dark eye sockets, brow ridge, nose shadow, mouth) drifting slowly around the frame,
// so the whole pipeline can be exercised without a camera or a person.
//
//   GenerateSampleVideo <output.avi> [faces]
//
// Build it against OpenCV core, imgproc and videoio, e.g. from a developer prompt:
//   cl /EHsc /MD /O2 /I <opencv>\include GenerateSampleVideo.cpp
//      /link /LIBPATH:<opencv>\x64\vc16\lib opencv_core452.lib opencv_imgproc452.lib opencv_videoio452.lib
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
  void DrawFace(cv::Mat& ioFrame, int iCx, int iCy)
  {
    const int fw = 90;  // face half width
    const int fh = 120; // face half height

    // Head
    cv::ellipse(ioFrame, { iCx, iCy }, { fw, fh }, 0, 0, 360, cv::Scalar(150, 180, 220), -1, cv::LINE_AA);

    // Forehead highlight, cheeks
    cv::ellipse(ioFrame, { iCx, iCy - fh / 3 }, { fw * 3 / 4, fh / 3 }, 0, 0, 360, cv::Scalar(165, 195, 235), -1, cv::LINE_AA);
    cv::circle(ioFrame, { iCx - fw / 2, iCy + fh / 5 }, 22, cv::Scalar(160, 190, 230), -1, cv::LINE_AA);
    cv::circle(ioFrame, { iCx + fw / 2, iCy + fh / 5 }, 22, cv::Scalar(160, 190, 230), -1, cv::LINE_AA);

    const int eyeY = iCy - fh / 5;
    const int eyeDX = fw / 2;

    // Brow ridge shadow, eyebrows
    cv::ellipse(ioFrame, { iCx, eyeY - 26 }, { fw * 4 / 5, 14 }, 0, 0, 360, cv::Scalar(120, 145, 185), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx - eyeDX, eyeY - 20 }, { 26, 7 }, 0, 0, 360, cv::Scalar(60, 70, 90), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx + eyeDX, eyeY - 20 }, { 26, 7 }, 0, 0, 360, cv::Scalar(60, 70, 90), -1, cv::LINE_AA);

    // Eye sockets darker than the cheeks, then whites and pupils
    cv::ellipse(ioFrame, { iCx - eyeDX, eyeY }, { 26, 16 }, 0, 0, 360, cv::Scalar(110, 130, 165), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx + eyeDX, eyeY }, { 26, 16 }, 0, 0, 360, cv::Scalar(110, 130, 165), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx - eyeDX, eyeY }, { 20, 11 }, 0, 0, 360, cv::Scalar(235, 240, 245), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx + eyeDX, eyeY }, { 20, 11 }, 0, 0, 360, cv::Scalar(235, 240, 245), -1, cv::LINE_AA);
    cv::circle(ioFrame, { iCx - eyeDX, eyeY }, 7, cv::Scalar(40, 40, 40), -1, cv::LINE_AA);
    cv::circle(ioFrame, { iCx + eyeDX, eyeY }, 7, cv::Scalar(40, 40, 40), -1, cv::LINE_AA);

    // Nose with a shadow on one side
    const int noseY = iCy + fh / 8;
    cv::line(ioFrame, { iCx, eyeY + 12 }, { iCx - 8, noseY }, cv::Scalar(120, 145, 185), 4, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx, noseY + 6 }, { 14, 8 }, 0, 0, 180, cv::Scalar(115, 140, 180), -1, cv::LINE_AA);

    // Mouth darker than everything around it
    const int mouthY = iCy + fh / 2;
    cv::ellipse(ioFrame, { iCx, mouthY }, { 34, 12 }, 0, 0, 360, cv::Scalar(70, 70, 140), -1, cv::LINE_AA);
    cv::ellipse(ioFrame, { iCx, mouthY - 3 }, { 30, 5 }, 0, 0, 360, cv::Scalar(50, 50, 100), -1, cv::LINE_AA);
  }
}

int main(int argc, char** argv)
{
  const char* path = argc > 1 ? argv[1] : "sample.avi";
  const int faceCount = std::clamp(argc > 2 ? std::atoi(argv[2]) : 1, 1, 2);

  const int width = 640;
  const int height = 480;
  const int frames = 900;
  const double fps = 30.0;

  cv::VideoWriter writer(path, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), fps, { width, height });
  if (!writer.isOpened())
  {
    std::printf("cannot open writer: %s\n", path);
    return 1;
  }

  for (int f = 0; f < frames; ++f)
  {
    cv::Mat frame(height, width, CV_8UC3, cv::Scalar(70, 80, 90));

    const double t = f / fps;

    for (int face = 0; face < faceCount; ++face)
    {
      // Slow drift, always fully inside the frame
      const int baseX = faceCount == 1 ? width / 2 : width / 2 + (face == 0 ? -160 : 160);
      const int amplitude = faceCount == 1 ? 80 : 30;

      const int cx = baseX + static_cast<int>(amplitude * std::sin(0.4 * t + face));
      const int cy = height / 2 + static_cast<int>(30.0 * std::sin(0.25 * t + 1.0 + face));

      DrawFace(frame, cx, cy);
    }

    // A soft blur takes the cartoon edge off, which the cascade prefers
    cv::GaussianBlur(frame, frame, { 5, 5 }, 0.0);

    writer.write(frame);
  }

  writer.release();
  std::printf("wrote %d frames with %d face(s) to %s\n", frames, faceCount, path);
  return 0;
}
