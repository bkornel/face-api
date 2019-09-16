#include "FaceApi.h"
#include "Common/Configuration.h"
#include "Framework/Stopwatch.h"
#include "Framework/UtilOCV.h"

#include <easyloggingpp/easyloggingpp.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

#include <iomanip>
#include <sstream>
#include <vector>

#include <cstdio>
#include <jni.h>
#include <android/log.h>

//#define FACE_PROFILE

namespace face_jni {
    constexpr const char *cModuleName = "FACE_JNI";
    constexpr android_LogPriority cLogLevel = ANDROID_LOG_DEBUG;

    template<typename T>
    void log(T iMessage) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << iMessage;
        __android_log_print(cLogLevel, cModuleName, "%s", ss.str().c_str());
    }

    cv::Mat ARGB_2_BGRA(const cv::Mat &iImage) {
        CV_Assert(iImage.channels() == 4);

        cv::Mat converted(iImage.rows, iImage.cols, iImage.type());
        int fromTo[] = {0, 3, 1, 2, 2, 1, 3, 0};
        cv::mixChannels(&iImage, 1, &converted, 1, fromTo, 4);
        return converted;
    }
}

extern "C"
{
JNIEXPORT jint JNICALL
Java_com_face_common_Native_initialize(JNIEnv *iEnv, jobject /*iThis*/, jstring iPath) {
    if (!iEnv) return -1;

    face_jni::log("INITIALIZING NATIVE SIDE");

    if (!iPath) {
        face_jni::log("- Empty or invalid working directory is sent from Java side");
        face_jni::log("- Native side is not initialized correctly");
        return 1;
    }

    static const cv::FileNode sSettingsNode;

    const jsize pathLength = iEnv->GetStringUTFLength(iPath);
    const char *pathChars = iEnv->GetStringUTFChars(iPath, nullptr);
    const std::string workingDirectory(pathChars, static_cast<unsigned >(pathLength));
    iEnv->ReleaseStringUTFChars(iPath, pathChars);

    face_jni::log("- Working directory sent from Java side: " + workingDirectory);
    face::FaceApi::GetInstance().SetWorkingDirectory(workingDirectory);

    if (face::FaceApi::GetInstance().Initialize(sSettingsNode) != fw::ErrorCode::OK) {
        face_jni::log("- FaceApi cannot be initialized (check its settings file first)");
        face_jni::log("- Native side is not initialized correctly");
        return 1;
    }

    face_jni::log("- Native side is initialized successfully");
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_face_common_Native_reset(JNIEnv */*iEnv*/, jobject /*iThis*/) {
    face_jni::log("RESETTING NATIVE SIDE");
    face::FaceApi::GetInstance().Clear();
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_face_common_Native_process(JNIEnv *iEnv, jobject /*iThis*/, jint iRotation, jint iWidth,
                                    jint iHeight, jbyteArray iYUV, jintArray iARGB) {
    if (!iEnv) return -1;

    int retVal = 0;

#ifdef FACE_PROFILE
    face::Stopwatch stopwatch;
    stopwatch.Start();
#endif

    // YUV420sp to BGR conversion
    jbyte *jni_yuv = iEnv->GetByteArrayElements(iYUV, nullptr);
    if (!jni_yuv || iEnv->ExceptionOccurred()) {
        return -1;
    }

    cv::Mat bgr;
    cv::Mat jni_yuv_mat(iHeight + iHeight / 2, iWidth, CV_8UC1, (unsigned char *) jni_yuv);
    if (!jni_yuv_mat.empty()) {
        cv::cvtColor(jni_yuv_mat, bgr, cv::COLOR_YUV420sp2BGR, 3);

#ifdef FACE_PROFILE
        face_jni::log("[FACE_PROFILE] cvtColor: " << stopwatch.GetElapsedTimeMilliSec(false));
        stopwatch.Reset();
#endif

        // Rotate image regarding the display orientation
        fw::ocv::rotate_mat(bgr, bgr, iRotation);

#ifdef FACE_PROFILE
        face_jni::log("[FACE_PROFILE] rotate_mat: " << stopwatch.GetElapsedTimeMilliSec(false));
        stopwatch.Reset();
#endif
    }

    if (!bgr.empty() && face::FaceApi::GetInstance().IsRunning()) {
        face::FaceApi::GetInstance().PushCameraFrame(bgr);

        cv::Mat resultImage;
        if (face::FaceApi::GetInstance().GetResultImage(resultImage) == fw::ErrorCode::OK) {
            if (!resultImage.empty() && bgr.size() == resultImage.size()) {
                jint *jni_argb = iEnv->GetIntArrayElements(iARGB, nullptr);
                if (jni_argb && !iEnv->ExceptionOccurred()) {
                    cv::Mat jni_argb_mat(bgr.rows, bgr.cols, CV_8UC4, (unsigned char *) jni_argb);

                    cv::cvtColor(resultImage, jni_argb_mat, cv::COLOR_BGR2BGRA, 4);
                    jni_argb_mat = face_jni::ARGB_2_BGRA(jni_argb_mat);

                    iEnv->ReleaseIntArrayElements(iARGB, jni_argb, 0);
                } else {
                    retVal = -1;
                }
            } else {
                retVal = 1;
            }
        } else {
            retVal = 2;
        }
    } else {
        retVal = 3;
    }

#ifdef FACE_PROFILE
    face_jni::log("[FACE_PROFILE] ARGB_2_BGRA: " << stopwatch.GetElapsedTimeMilliSec(false));
    stopwatch.Reset();
#endif

    iEnv->ReleaseByteArrayElements(iYUV, jni_yuv, 0);

    return retVal;
}
} // extern "C"
