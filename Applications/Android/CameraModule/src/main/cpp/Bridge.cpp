#include "Framework/Imaging/MatExtensions.h"
#include "Framework/ErrorCode.h"
#include "FaceApi.h"
#include "FaceResultBuffer.h"
#include "Common/Configuration.h"
#include "Framework/Stopwatch.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <iomanip>
#include <sstream>
#include <vector>

#include <cstdio>
#include <jni.h>
#include <android/log.h>

//#define FACE_PROFILE

namespace face_jni
{
  constexpr const char* cModuleName = "FACE_JNI";
  constexpr android_LogPriority cLogLevel = ANDROID_LOG_DEBUG;

  template <typename T>
  void log(T iMessage)
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << iMessage;
    __android_log_print(cLogLevel, cModuleName, "%s", ss.str().c_str());
  }

  /// @brief Prefix plus value. The FACE_PROFILE call sites used to write
  /// log("text: " << value), which does not compile: log() takes a single
  /// argument and there is no operator<< on a string literal.
  template <typename T>
  void log(const char* iPrefix, T iValue)
  {
    std::stringstream ss;
    ss << iPrefix << std::fixed << std::setprecision(2) << iValue;
    __android_log_print(cLogLevel, cModuleName, "%s", ss.str().c_str());
  }

  // Number of bytes an NV21 / YUV420sp frame of the given size occupies.
  inline jsize yuv420sp_size(jint iWidth, jint iHeight)
  {
    return static_cast<jsize>(iWidth) * iHeight * 3 / 2;
  }

}

extern "C" {
JNIEXPORT jint JNICALL
Java_com_face_common_Native_initialize(JNIEnv* iEnv, jobject /*iThis*/, jstring iPath)
{
  if (!iEnv) return -1;

  face_jni::log("INITIALIZING NATIVE SIDE");

  if (!iPath)
  {
    face_jni::log("- Empty or invalid working directory is sent from Java side");
    face_jni::log("- Native side is not initialized correctly");
    return 1;
  }

  static const cv::FileNode sSettingsNode;

  const jsize pathLength = iEnv->GetStringUTFLength(iPath);
  const char* pathChars = iEnv->GetStringUTFChars(iPath, nullptr);
  const std::string workingDirectory(pathChars, static_cast<unsigned>(pathLength));
  iEnv->ReleaseStringUTFChars(iPath, pathChars);

  face_jni::log("- Working directory sent from Java side: " + workingDirectory);
  face::FaceApi::GetInstance().SetWorkingDirectory(workingDirectory);

  if (face::FaceApi::GetInstance().Initialize(sSettingsNode) != fw::ErrorCode::OK)
  {
    face_jni::log("- FaceApi cannot be initialized (check its settings file first)");
    face_jni::log("- Native side is not initialized correctly");
    return 1;
  }

  face_jni::log("- Native side is initialized successfully");
  return 0;
}

JNIEXPORT jint JNICALL
Java_com_face_common_Native_reset(JNIEnv* /*iEnv*/, jobject /*iThis*/)
{
  face_jni::log("RESETTING NATIVE SIDE");
  face::FaceApi::GetInstance().Clear();
  return 0;
}

/// @brief Copies the overlay data of the last processed frame into iBuffer.
/// @return the number of faces written, 0 when there is nothing to draw, negative on error.
///
/// This is the path to use for drawing the overlay with Canvas or OpenGL: it moves a couple
/// of kilobytes of geometry per frame instead of a full ARGB frame, and the camera preview
/// never has to make a round trip through the CPU to be displayed. The layout is defined by
/// face::result_buffer, and FaceOverlayData on the Java side reads it back.
JNIEXPORT jint JNICALL
Java_com_face_common_Native_getResults(JNIEnv* iEnv, jobject /*iThis*/, jfloatArray iBuffer)
{
  if (!iEnv || !iBuffer) return -1;

  const jsize capacity = iEnv->GetArrayLength(iBuffer);

  face::FaceResults results;
  if (face::FaceApi::GetInstance().GetResults(results) != fw::ErrorCode::OK)
  {
    results.clear();
  }

  std::vector<jfloat> buffer(static_cast<std::size_t>(capacity), 0.0F);
  const int faceCount = face::result_buffer::pack(results, buffer.data(), static_cast<int>(capacity));

  if (faceCount < 0)
  {
    face_jni::log("- The result buffer is too small for a single face");
    return -1;
  }

  const jsize written =
    static_cast<jsize>(face::result_buffer::cHeaderFloats + (faceCount * face::result_buffer::cFaceStride));

  iEnv->SetFloatArrayRegion(iBuffer, 0, written, buffer.data());

  return faceCount;
}

JNIEXPORT jint JNICALL
Java_com_face_common_Native_process(JNIEnv* iEnv, jobject /*iThis*/, jint iRotation, jint iWidth, jint iHeight, jbyteArray iYUV, jintArray iARGB)
{
  if (!iEnv) return -1;

  int retVal = 0;

#ifdef FACE_PROFILE
  fw::Stopwatch stopwatch;
  stopwatch.Start();
#endif

  if (!iYUV || iWidth <= 0 || iHeight <= 0)
  {
    return -1;
  }

  // Never trust the sizes the Java side passed in: they decide how much of the
  // array the cv::Mat below reads.
  if (iEnv->GetArrayLength(iYUV) < face_jni::yuv420sp_size(iWidth, iHeight))
  {
    face_jni::log("- The YUV array is smaller than the given frame size");
    return -1;
  }

  // iARGB is optional. Leaving it out skips the rendered frame altogether, which is what
  // a host that draws the overlay itself wants: no full frame crosses the JNI boundary.
  if (iARGB && iEnv->GetArrayLength(iARGB) < static_cast<jsize>(iWidth) * iHeight)
  {
    face_jni::log("- The ARGB array is smaller than the given frame size");
    return -1;
  }

  // YUV420sp to BGR conversion
  jbyte* jni_yuv = iEnv->GetByteArrayElements(iYUV, nullptr);
  if (!jni_yuv)
  {
    return -1;
  }

  if (iEnv->ExceptionOccurred())
  {
    // The array was pinned, so it has to be released even on this path.
    iEnv->ReleaseByteArrayElements(iYUV, jni_yuv, JNI_ABORT);
    return -1;
  }

  cv::Mat bgr;
  cv::Mat jni_yuv_mat(iHeight + iHeight / 2, iWidth, CV_8UC1, (unsigned char*)jni_yuv);
  if (!jni_yuv_mat.empty())
  {
    cv::cvtColor(jni_yuv_mat, bgr, cv::COLOR_YUV420sp2BGR, 3);

#ifdef FACE_PROFILE
    face_jni::log("[FACE_PROFILE] cvtColor ms: ", stopwatch.GetElapsedTimeMilliSec(false));
    stopwatch.Reset();
#endif

    // Rotate image regarding the display orientation
    fw::rotate_mat(bgr, bgr, iRotation);

#ifdef FACE_PROFILE
    face_jni::log("[FACE_PROFILE] rotate_mat ms: ", stopwatch.GetElapsedTimeMilliSec(false));
    stopwatch.Reset();
#endif
  }

  if (!bgr.empty() && face::FaceApi::GetInstance().IsRunning())
  {
    face::FaceApi::GetInstance().PushCameraFrame(bgr);

    cv::Mat resultImage;
    if (!iARGB)
    {
      // Nothing to hand back, the caller draws from getResults()
    }
    else if (face::FaceApi::GetInstance().GetResultImage(resultImage) == fw::ErrorCode::OK)
    {
      if (!resultImage.empty() && bgr.size() == resultImage.size())
      {
        jint* jni_argb = iEnv->GetIntArrayElements(iARGB, nullptr);
        if (jni_argb && !iEnv->ExceptionOccurred())
        {
          cv::Mat jni_argb_mat(bgr.rows, bgr.cols, CV_8UC4, (unsigned char*)jni_argb);

          // Writes straight into the pinned Java int[]. This is already the
          // layout Bitmap.setPixels() expects: an ARGB_8888 int is
          // 0xAARRGGBB, which on a little endian device is B,G,R,A in
          // memory, exactly what BGR2BGRA produces.
          //
          // A channel swapping ARGB_2_BGRA() call used to follow this line.
          // It assigned a freshly allocated cv::Mat to the local variable,
          // so it never wrote anything back into the Java array. It only
          // cost a full frame allocation and a mixChannels() per frame.
          cv::cvtColor(resultImage, jni_argb_mat, cv::COLOR_BGR2BGRA, 4);

          iEnv->ReleaseIntArrayElements(iARGB, jni_argb, 0);
        }
        else
        {
          retVal = -1;
        }
      }
      else
      {
        retVal = 1;
      }
    }
    else
    {
      retVal = 2;
    }
  }
  else
  {
    retVal = 3;
  }

#ifdef FACE_PROFILE
  face_jni::log("[FACE_PROFILE] cvtColor to ARGB ms: ", stopwatch.GetElapsedTimeMilliSec(false));
  stopwatch.Reset();
#endif

  // JNI_ABORT, not 0: the YUV frame is only read here, so there is no reason to
  // copy the whole buffer back to the Java array on every frame.
  iEnv->ReleaseByteArrayElements(iYUV, jni_yuv, JNI_ABORT);

  return retVal;
}
} // extern "C"
