LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Dependencies.mk

# Face API headers and sources
# =======================================================
NATIVE_PATH := $(LOCAL_PATH)/../../../Coding/Native
BRIDGE_PATH := $(LOCAL_PATH)/../../../Coding/Bridge

NATIVE_DIRS := 										\
	$(BRIDGE_PATH)									\
	$(NATIVE_PATH) 									\
	$(NATIVE_PATH)/3rdparty							\
	$(NATIVE_PATH)/3rdparty/clm						\
	$(NATIVE_PATH)/3rdparty/easyloggingpp			\
	$(NATIVE_PATH)/Common							\
	$(NATIVE_PATH)/Framework						\
	$(NATIVE_PATH)/Messages							\
	$(NATIVE_PATH)/Modules							\
	$(NATIVE_PATH)/Modules/FaceDetection			\
	$(NATIVE_PATH)/Modules/ImageQueue				\
	$(NATIVE_PATH)/Modules/UserHistory				\
	$(NATIVE_PATH)/Modules/UserManager				\
	$(NATIVE_PATH)/Modules/UserProcessor			\
	$(NATIVE_PATH)/Modules/UserProcessor/HeadPose	\
	$(NATIVE_PATH)/Modules/UserProcessor/ShapeNorm	\
	$(NATIVE_PATH)/Modules/UserProcessor/ShapeModel	\
	$(NATIVE_PATH)/Modules/Visualizer				\
	$(NATIVE_PATH)/Shape							\
	$(NATIVE_PATH)/User

# Collect source files
LOCAL_SRC_FILES := $(foreach dir, $(NATIVE_DIRS), $(wildcard $(dir)/*.cpp))
LOCAL_SRC_FILES += $(foreach dir, $(NATIVE_DIRS), $(wildcard $(dir)/*.cc))
LOCAL_SRC_FILES += $(foreach dir, $(NATIVE_DIRS), $(wildcard $(dir)/*.c))

# Collect headers
LOCAL_C_INCLUDES := 									\
	$(NATIVE_DIRS)										\
	$(OPENCV_HEADERS)/include							\
	$(OPENCV_HEADERS)/include/opencv					\
	$(OPENCV_HEADERS)/include/opencv2

# Build native shared library
LOCAL_MODULE := face_native
LOCAL_LDLIBS += -lm -llog -landroid -ldl
LOCAL_CPPFLAGS += -DELPP_THREAD_SAFE -DELPP_FORCE_USE_STD_THREAD
LOCAL_CFLAGS += -std=c++11 -fopenmp
LOCAL_LDFLAGS += -fopenmp

include $(BUILD_SHARED_LIBRARY)
