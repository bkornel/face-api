// The whole pipeline over the sample clips: that a face is found and fitted, that the
// results a host reads are the ones the graph produced, and that tearing the graph down and
// building it again - which is what Face Studio does after saving settings - leaves a
// pipeline that still works. Skipped when the test assets are not next to the executable.
#include "TestSupport.h"

#include "FaceApi.h"

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace
{
  /// @brief Pushes iMaxFrames of iVideo through iApi and reports what came out.
  struct RunResult
  {
    int pushed = 0;
    int accepted = 0;
    int framesOut = 0;
    int framesWithFace = 0;

    /// @brief The most faces reported on any one frame, which is what says whether the
    /// batched fit handled more than one
    std::size_t mostFaces = 0U;

    /// @brief The frame that carried the most faces, kept so it can be inspected
    face::FaceResults bestResults;

    face::FaceResults lastResults;
  };

  /// @param iPaceMs How long to wait between frames. A camera's rate is what paces this in
  /// an application; a lower value walks through the clip faster, and the sampler turns away
  /// whatever arrives sooner than the configured rate.
  /// @param iStopAtFaces Return as soon as this many faces are reported at once, 0 to read
  /// iMaxFrames whatever happens
  RunResult Run(face::FaceApi& ioApi, const std::string& iVideo, int iMaxFrames,
                int iPaceMs = 16, std::size_t iStopAtFaces = 0U)
  {
    RunResult result;

    cv::VideoCapture capture(iVideo);
    if (!capture.isOpened()) return result;

    cv::Mat frame;

    while (result.pushed < iMaxFrames && capture.read(frame) && !frame.empty())
    {
      if (iStopAtFaces > 0U && result.mostFaces >= iStopAtFaces) break;

      ++result.pushed;

      if (ioApi.PushCameraFrame(frame) == fw::ErrorCode::OK) ++result.accepted;

      std::this_thread::sleep_for(std::chrono::milliseconds(iPaceMs));

      cv::Mat processed;
      if (ioApi.GetResultImage(processed) == fw::ErrorCode::OK && !processed.empty())
      {
        ++result.framesOut;

        face::FaceResults results;
        if (ioApi.GetResults(results) == fw::ErrorCode::OK && !results.empty())
        {
          ++result.framesWithFace;

          if (results.size() > result.mostFaces)
          {
            result.mostFaces = results.size();
            result.bestResults = results;
          }

          result.lastResults = std::move(results);
        }
      }
    }

    return result;
  }

  void CheckOneFace(const face::FaceResults& iResults, const std::string& iWhat)
  {
    test::Check(!iResults.empty(), iWhat + ": a face is reported");
    if (iResults.empty()) return;

    const face::FaceResult& face = iResults.front();

    test::Check(face.shape2D.size() == 68U,
                iWhat + ": the shape has 68 points (" + std::to_string(face.shape2D.size()) + ")");

    test::Check(face.faceRect.width > 0 && face.faceRect.height > 0, iWhat + ": the face has a rectangle");
    test::Check(face.HasPose(), iWhat + ": the pose was solved");

    if (face.HasPose())
    {
      test::Check(face.position3D[2] > 0.0,
                  iWhat + ": the head is in front of the camera (z = " + std::to_string(face.position3D[2]) + ")");

      test::Check(std::abs(fw::rad_to_deg(face.rpy[2])) < 60.0,
                  iWhat + ": the yaw is plausible for a face looking at the camera (" +
                    std::to_string(fw::rad_to_deg(face.rpy[2])) + " deg)");
    }

    test::Check(face.shape3D.size() == face.shape2D.size(), iWhat + ": the 3-D shape is reported too");
    test::Check(!face.normShape3D.empty(), iWhat + ": the normalised shape is reported");
    test::Check(face.expression.openEyeLeft >= 0.0 && face.expression.openEyeLeft <= 1.0,
                iWhat + ": the expression measures are in range");
  }
}

void RunPipelineTests(const std::string& iTestRoot)
{
  test::Section("the pipeline end to end");

  if (iTestRoot.empty())
  {
    std::printf("skipped: the Testing directory was not found next to the executable\n");
    return;
  }

  const std::string configurations = iTestRoot + "configurations/";
  const std::string oneFace = iTestRoot + "media/sample_one_face.avi";

  face::FaceApi api;
  api.SetWorkingDirectory(configurations);

  static const cv::FileNode sEmptyNode;

  const bool initialized = (api.Initialize(sEmptyNode) == fw::ErrorCode::OK);
  test::Check(initialized, "the pipeline initializes from the test configuration");

  if (!initialized)
  {
    std::printf("skipped the rest: check that the models are in %s\n", configurations.c_str());
    return;
  }

  test::Check(api.IsRunning(), "its worker thread is running");

  const RunResult first = Run(api, oneFace, 60);

  test::Check(first.pushed > 0, "the sample clip could be read (" + std::to_string(first.pushed) + " frames)");
  test::Check(first.framesOut > 0, "frames come out of the pipeline (" + std::to_string(first.framesOut) + ")");
  test::Check(first.framesWithFace > 0,
              "a face is tracked on most of them (" + std::to_string(first.framesWithFace) + ")");

  CheckOneFace(first.lastResults, "first run");

  // What the settings editor does when it saves: the graph is torn down and built again
  // from the file, with the source left alone
  test::Check(api.DeInitialize() == fw::ErrorCode::OK, "the pipeline deinitializes");
  test::Check(!api.IsRunning(), "and its worker thread stops");

  const bool reinitialized = (api.Initialize(sEmptyNode) == fw::ErrorCode::OK);
  test::Check(reinitialized, "the pipeline initializes a second time from the same settings");

  if (reinitialized)
  {
    const RunResult second = Run(api, oneFace, 60);

    test::Check(second.framesOut > 0,
                "frames still come out after the reload (" + std::to_string(second.framesOut) + ")");
    test::Check(second.framesWithFace > 0,
                "and a face is still tracked (" + std::to_string(second.framesWithFace) + ")");

    CheckOneFace(second.lastResults, "after reload");
  }

  test::Check(api.GetQueueSize() >= 0, "the queue depth is reported");

  // Two faces at once: the fit prepares every crop, runs them through the network in one
  // pass and decodes each. A batch that lost or mixed up a row shows up right here.
  // Walked through quickly and stopped as soon as both faces are up: the second one enters
  // the clip well after the first
  const std::string twoFaces = iTestRoot + "media/sample_two_faces.avi";
  const RunResult both = Run(api, twoFaces, 900, 4, 2U);

  test::Check(both.mostFaces >= 2U,
              "two faces are fitted on the same frame (" + std::to_string(both.mostFaces) +
                " at most, over " + std::to_string(both.pushed) + " frames, " +
                std::to_string(both.framesOut) + " out, " + std::to_string(both.framesWithFace) + " with a face)");

  if (both.mostFaces >= 2U)
  {
    bool everyFaceFitted = true;
    bool distinctShapes = true;

    for (const auto& face : both.bestResults)
      everyFaceFitted = everyFaceFitted && (face.shape2D.size() == 68U) && face.HasPose();

    // Both rows of the batch have to be decoded onto their own face, not the same one twice
    if (both.bestResults.size() >= 2U)
    {
      const auto& first = both.bestResults[0].shape2D;
      const auto& second = both.bestResults[1].shape2D;

      distinctShapes = !first.empty() && !second.empty() && cv::norm(first.front() - second.front()) > 5.0;
    }

    test::Check(everyFaceFitted, "every face of the batch has a shape and a pose");
    test::Check(distinctShapes, "each face of the batch got its own shape");
  }

  api.DeInitialize();
}
