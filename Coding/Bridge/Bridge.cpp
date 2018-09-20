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

#include "FaceApp.h"
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
	std::string ConvertJString(JNIEnv* env, jstring str)
	{
	   if (!str) std::string();

	   const jsize len = env->GetStringUTFLength(str);
	   const char* strChars = env->GetStringUTFChars(str, (jboolean *)0);
	   
	   std::string Result(strChars, len);
	   
	   env->ReleaseStringUTFChars(str, strChars);

	   return Result;
	}
	
	JNIEXPORT jint JNICALL 
	Java_com_yalantis_cameramodule_fragment_CameraFragment_NativeReset(JNIEnv* env, jobject thiz)
	{
		FACE_LOG_JNI("RESET NATIVE SIDE");
		face::FaceApp::GetInstance().Clear();
		return 0;
	}
	
	JNIEXPORT jint JNICALL 
	Java_com_yalantis_cameramodule_activity_CameraActivity_NativeInitialize(JNIEnv* env, jobject thiz, jstring path)
	{
		static const cv::FileNode sSettingsNode;

		FACE_LOG_JNI("INITIALIZING NATIVE SIDE");
		const std::string& workingDirectory = ConvertJString(env, path);
		
		FACE_LOG_JNI("- Working directory: " << workingDirectory);
		face::FaceApp::GetInstance().SetWorkingDirectory(workingDirectory);

		if (face::FaceApp::GetInstance().Initialize(sSettingsNode) != fw::ErrorCode::OK) 
		{
			FACE_LOG_JNI("- Native side is not initialized correctly");
			return 1;
		}
		
		FACE_LOG_JNI("- Native side is initialized");
		return 0;
	}
	
	JNIEXPORT jint JNICALL 
	Java_com_yalantis_cameramodule_control_CameraPreview_NativeProcess(JNIEnv* env, jobject thiz, jint rotation, jint width, jint height, jbyteArray yuv, jintArray argb)
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
		cv::cvtColor(jni_yuv_mat, bgr, CV_YUV420sp2BGR, 3);
		
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
		
		if (face::FaceApp::GetInstance().IsRunning()) 
		{
			CV_Assert(!bgr.empty());

			face::FaceApp::GetInstance().PushCameraFrame(bgr);
			
			cv::Mat resultImage;
			if(face::FaceApp::GetInstance().GetResultImage(resultImage) == fw::ErrorCode::OK)
			{
				CV_Assert(!resultImage.empty());
				
				if (bgr.size() == resultImage.size()) 
				{
					jint* jni_argb = env->GetIntArrayElements(argb, 0);
					cv::Mat jni_argb_mat(bgr.rows, bgr.cols, CV_8UC4, (unsigned char *)jni_argb);

					cv::cvtColor(resultImage, jni_argb_mat, CV_BGR2BGRA, 4);
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
