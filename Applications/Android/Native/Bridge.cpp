#include <stdio.h>
#include <jni.h>
#include <android/log.h>

#include <iomanip>
#include <sstream>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <easyloggingpp/easyloggingpp.h>

#include "FaceApi.h"
#include "Common/Configuration.h"
#include "Framework/Stopwatch.h"
#include "Framework/UtilOCV.h"

//#define FACE_PROFILE
#define FACE_APP_NAME_JNI "JNIFACE"
#define FACE_LOG_LEVEL_JNI ANDROID_LOG_DEBUG
#define FACE_LOG_JNI(message) 																\
	{																						\
		std::stringstream ss;																\
		ss << std::fixed << std::setprecision(2) << message;								\
		__android_log_print(FACE_LOG_LEVEL_JNI, FACE_APP_NAME_JNI, "%s", ss.str().c_str());	\
	}

namespace
{
	cv::Mat ARGB_2_BGRA(const cv::Mat& mat)
	{
		CV_Assert(mat.channels() == 4);

		cv::Mat newMat(mat.rows, mat.cols, mat.type());
		int from_to[] = { 0, 3, 1, 2, 2, 1, 3, 0 };
		cv::mixChannels(&mat, 1, &newMat, 1, from_to, 4);
		return newMat;
	}
}
	
extern "C"
{
	JNIEXPORT jint JNICALL 
	Java_com_face_fragment_CameraFragment_NativeReset(JNIEnv* env, jobject thiz)
	{
		FACE_LOG_JNI("RESET NATIVE SIDE");
		face::FaceApi::GetInstance().Clear();
		return 0;
	}
	
	JNIEXPORT jint JNICALL 
	Java_com_face_activity_CameraActivity_NativeInitialize(JNIEnv* env, jobject thiz, jstring path)
	{
		FACE_LOG_JNI("INITIALIZING NATIVE SIDE");

		if (!path)
		{
			FACE_LOG_JNI("- Native side is not initialized correctly");
			return 1;
		}

		static const cv::FileNode sSettingsNode;
		
		const jsize pathLength = env->GetStringUTFLength(path);
		const char* pathChars = env->GetStringUTFChars(path, (jboolean *)0);
		const std::string workingDirectory(pathChars, pathLength);
		env->ReleaseStringUTFChars(path, pathChars);

		FACE_LOG_JNI("- Working directory: " << workingDirectory);
		face::FaceApi::GetInstance().SetWorkingDirectory(workingDirectory);

		if (face::FaceApi::GetInstance().Initialize(sSettingsNode) != fw::ErrorCode::OK) 
		{
			FACE_LOG_JNI("- Native side is not initialized correctly");
			return 1;
		}
		
		FACE_LOG_JNI("- Native side is initialized");
		return 0;
	}
	
	JNIEXPORT jint JNICALL 
	Java_com_face_control_CameraView_NativeProcess(JNIEnv* env, jobject thiz, jint rotation, jint width, jint height, jbyteArray yuv, jintArray argb)
	{
		int retVal = 0;
		
		#ifdef FACE_PROFILE
		face::Stopwatch stopwatch;
		stopwatch.Start();
		#endif
		
		// YUV420sp to BGR conversion
		jbyte* jni_yuv  = env->GetByteArrayElements(yuv, 0);
		cv::Mat jni_yuv_mat(height + height/2, width, CV_8UC1, (unsigned char *)jni_yuv);

		cv::Mat bgr;
		cv::cvtColor(jni_yuv_mat, bgr, cv::COLOR_YUV420sp2BGR, 3);
		
		#ifdef FACE_PROFILE
		FACE_LOG_JNI("[FACE_PROFILE] cvtColor: " << stopwatch.GetElapsedTimeMilliSec(false));
		stopwatch.Reset();
		#endif
		
		// Rotate image regarding the display orientation
		fw::ocv::rotate_mat(bgr, bgr, rotation);
		
		#ifdef FACE_PROFILE
		FACE_LOG_JNI("[FACE_PROFILE] rotate_mat: " << stopwatch.GetElapsedTimeMilliSec(false));
		stopwatch.Reset();
		#endif
		
		if (face::FaceApi::GetInstance().IsRunning()) 
		{
			CV_Assert(!bgr.empty());

			face::FaceApi::GetInstance().PushCameraFrame(bgr);
			
			cv::Mat resultImage;
			if(face::FaceApi::GetInstance().GetResultImage(resultImage) == fw::ErrorCode::OK)
			{
				CV_Assert(!resultImage.empty());
				
				if (bgr.size() == resultImage.size()) 
				{
					jint* jni_argb = env->GetIntArrayElements(argb, 0);
					cv::Mat jni_argb_mat(bgr.rows, bgr.cols, CV_8UC4, (unsigned char *)jni_argb);

					cv::cvtColor(resultImage, jni_argb_mat, cv::COLOR_BGR2BGRA, 4);
					jni_argb_mat = ARGB_2_BGRA(jni_argb_mat);
				
					env->ReleaseIntArrayElements(argb, jni_argb, 0);
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
		FACE_LOG_JNI("[FACE_PROFILE] ARGB_2_BGRA: " << stopwatch.GetElapsedTimeMilliSec(false));
		stopwatch.Reset();
		#endif
		
		env->ReleaseByteArrayElements(yuv, jni_yuv, 0);
	
		return retVal;
	}
}
