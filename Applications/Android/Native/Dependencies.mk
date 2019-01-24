LOCAL_PATH := $(call my-dir)

DEPENDENCIES_PATH := $(LOCAL_PATH)/../../../../3rdparty

# OpenCV headers and libraries
# =======================================================
OPENCV_PATH := $(DEPENDENCIES_PATH)/opencv-4.0.1/android
OPENCV_HEADERS := $(OPENCV_PATH)/jni/include

include $(CLEAR_VARS)
OPENCV_LIB_TYPE := STATIC
OPENCV_INSTALL_MODULES := on
OPENCV_CAMERA_MODULES := off
include $(OPENCV_PATH)/jni/OpenCV.mk
