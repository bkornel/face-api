# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22)

# The shared library has the name: libnative.so
set(TARGET_NAME FaceNative)
set(TARGET_NAME_FACE_API FaceApi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Building native shared library")
message("- Output directory of ${TARGET_NAME}: '${CMAKE_BINARY_DIR}'")

# Setting some paths to 3rdparties and to the application logic
set(DEPENDECIES_DIR "${CMAKE_SOURCE_DIR}/../../../../3rdparty")
set(FACE_API_DIR    "${CMAKE_SOURCE_DIR}/../../../${TARGET_NAME_FACE_API}")
set(OPENCV_VERSION  "opencv-4.5.2" CACHE STRING "OpenCV directory name under 3rdparty")
set(OpenCV_DIR      "${DEPENDECIES_DIR}/${OPENCV_VERSION}/android/src/sdk/native/jni")

# Static OpenCV, see FaceApi/CMakeLists.txt
set(OpenCV_STATIC ON)

# Finding 3rparty libraries
find_package(OpenCV REQUIRED)
find_library(log-lib log)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${FACE_API_DIR})
include_directories(${FACE_API_DIR}/3rdparty)

link_directories(${CMAKE_BINARY_DIR}/${TARGET_NAME_FACE_API})

add_subdirectory(${FACE_API_DIR} ${CMAKE_BINARY_DIR}/${TARGET_NAME_FACE_API})

set(SOURCES;
  src/main/cpp/Bridge.cpp;
)

set(LIBRARIES;
  ${log-lib};
  ${OpenCV_LIBS};
  ${TARGET_NAME_FACE_API}
)

# Gradle automatically packages shared libraries with your APK
add_library(${TARGET_NAME} SHARED ${SOURCES})
target_link_libraries(${TARGET_NAME} ${LIBRARIES})
add_dependencies(${TARGET_NAME} ${TARGET_NAME_FACE_API})

# Print out some debug information regarding the inlcude directories and libraries
message("- Include directories of ${TARGET_NAME}:")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message("-- '${dir}'")
endforeach()

message("- Target link libraries of ${TARGET_NAME}: '${LIBRARIES}'")
