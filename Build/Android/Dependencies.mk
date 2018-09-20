LOCAL_PATH := $(call my-dir)

DEPENDENCIES_PATH := $(LOCAL_PATH)/../../../../3rdparty

# OpenCV headers and libraries
# =======================================================
OPENCV_HEADERS := $(DEPENDENCIES_PATH)/opencv-3.4.1/android/native/jni

include $(CLEAR_VARS)
OPENCV_INSTALL_MODULES := on
OPENCV_CAMERA_MODULES := off
include $(OPENCV_HEADERS)/OpenCV.mk
