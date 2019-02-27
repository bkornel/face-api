# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.6.0)

set(target_name native-lib)
set(OpenCV_DIR "../../../../3rdparty/opencv-4.0.1/android/src/sdk/native/jni")

find_package(OpenCV REQUIRED)
find_library(log-lib log)

set(sources;
  src/main/cpp/native-lib.cpp;
)

set(libraries;
  ${log-lib};
  ${OpenCV_LIBS};
)

# Gradle automatically packages shared libraries with your APK
add_library(${target_name} SHARED ${sources})

target_link_libraries(${target_name} ${libraries})
