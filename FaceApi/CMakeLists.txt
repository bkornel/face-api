cmake_minimum_required(VERSION 3.22)

# The shared library has the name: libFaceApi.so
set(TARGET_NAME FaceApi)

message("Building FaceApi shared library")
message("- Output directory of ${TARGET_NAME}: '${CMAKE_BINARY_DIR}/${TARGET_NAME}'")

# Setting some paths to 3rdparties and to the application logic
set(DEPENDECIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty")
set(OPENCV_VERSION  "opencv-4.5.2" CACHE STRING "OpenCV directory name under 3rdparty")
set(OpenCV_DIR      "${DEPENDECIES_DIR}/${OPENCV_VERSION}")

# Libraries have to determined based on the platform
set(LIBRARIES "")

# Checking and setting of platform specific configurations
message("- Checking CMAKE_SYSTEM_NAME: '${CMAKE_SYSTEM_NAME}'")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  message(FATAL_ERROR, "- Platform is not supported in cmake.")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Android")
  add_definitions(-DOS_ANDROID)
  message("- Checking CMAKE_ANDROID_ARCH_ABI: '${CMAKE_ANDROID_ARCH_ABI}'")
  
  set(OpenCV_DIR "${OpenCV_DIR}/android/src/sdk/native/jni")

  # Link OpenCV statically so everything ends up inside libFaceNative.so. This
  # removes the need to package any OpenCV shared object, which is what used to
  # break when the Android Gradle plugin was updated.
  set(OpenCV_STATIC ON)

  # Finding 3rparty libraries
  find_package(OpenCV REQUIRED)
  find_library(log-lib log)

  include_directories(${OpenCV_INCLUDE_DIRS})
  
  set(LIBRARIES;
    ${log-lib};
    ${OpenCV_LIBS};
  )
else()
  message(FATAL_ERROR, "- Platform is not supported.")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/clm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/easyloggingpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Framework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Messages)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/FaceDetection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/FirstModule)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/ImageQueue)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/LastModule)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserHistory)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserManager)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserProcessor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserProcessor/HeadPose)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserProcessor/ShapeModel)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/UserProcessor/ShapeNorm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/Visualizer)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/User)

# Sources are the same for all platforms
set(SOURCES;
  FaceApi.cpp;
  3rdparty/clm/CLM.cc;
  3rdparty/clm/FCheck.cc;
  3rdparty/clm/IO.cc;
  3rdparty/clm/Patch.cc;
  3rdparty/clm/PAW.cc;
  3rdparty/clm/PDM.cc;
  3rdparty/easyloggingpp/easyloggingpp.cpp;
  FaceResultBuffer.cpp;
  Configuration.cpp;
  Model/PoseGeometry.cpp;
  Model/FaceModel.cpp;
  Framework/Exception.cpp;
  Framework/Graph/FlowGraph.cpp;
  Framework/Messaging/Message.cpp;
  Framework/Messaging/MessageBus.cpp;
  Framework/Graph/Module.cpp;
  Framework/Profiler.cpp;
  Framework/Stopwatch.cpp;
  Framework/Thread.cpp;
  Framework/TimeExtensions.cpp;
  Framework/Imaging/Geometry.cpp;
  Framework/Imaging/Drawing.cpp;
  Framework/Imaging/Projection.cpp;
  Framework/Imaging/Transform.cpp;
  Framework/Imaging/Statistics.cpp;
  Framework/Text.cpp;
  Framework/Settings.cpp;
  Framework/Imaging/VideoWriter.cpp;
  Messages/ActiveUsersMessage.cpp;
  Messages/CommandMessage.cpp;
  Messages/ImageMessage.cpp;
  Messages/ImageSizeChangedMessage.cpp;
  Messages/RoiMessage.cpp;
  Messages/UserEntriesMessage.cpp;
  Modules/ModuleConnector.cpp;
  Modules/ModuleFactory.cpp;
  Modules/ModuleGraph.cpp;
  Modules/FaceDetection/FaceDetection.cpp;
  Modules/FirstModule/FirstModule.cpp;
  Modules/ImageQueue/ImageQueue.cpp;
  Modules/LastModule/LastModule.cpp;
  Modules/UserHistory/UserHistory.cpp;
  Modules/UserManager/UserManager.cpp;
  Modules/UserProcessor/UserProcessor.cpp;
  Modules/UserProcessor/HeadPose/PoseEstimationDispatcher.cpp;
  Modules/UserProcessor/ShapeModel/ClmWrapper.cpp;
  Modules/UserProcessor/ShapeModel/ShapeModel.cpp;
  Modules/UserProcessor/ShapeModel/ShapeModelDispatcher.cpp;
  Modules/UserProcessor/ShapeNorm/ShapeNormDispatcher.cpp;
  Modules/Visualizer/Visualizer.cpp;
  User/User.cpp;
  User/UserData.cpp;
)

add_library(${TARGET_NAME} STATIC ${SOURCES})
target_link_libraries(${TARGET_NAME} ${LIBRARIES})

# Print out some debug information regarding the inlcude directories and libraries
message("- Include directories of ${TARGET_NAME}:")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message("-- '${dir}'")
endforeach()

message("- Target link libraries of ${TARGET_NAME}: '${LIBRARIES}'")
