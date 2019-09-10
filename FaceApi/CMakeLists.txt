cmake_minimum_required(VERSION 3.6.0)

# The shared library has the name: libFaceApi.so
set(TARGET_NAME FaceApi)

message("Building FaceApi shared library")
message("- Output directory of ${TARGET_NAME}: '${CMAKE_BINARY_DIR}/${TARGET_NAME}'")

# Setting some paths to 3rdparties and to the application logic
set(DEPENDECIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty")
set(OpenCV_DIR      "${DEPENDECIES_DIR}/opencv-4.0.1")

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Framework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Messages)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/FaceDetection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/FirstModule)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Modules/Graph)
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
  Common/Configuration.cpp;
  Common/PoseUtil.cpp;
  Common/ShapeUtil.cpp;
  Framework/Exception.cpp;
  Framework/FlowGraph.cpp;
  Framework/Message.cpp;
  Framework/Module.cpp;
  Framework/Profiler.cpp;
  Framework/Stopwatch.cpp;
  Framework/Thread.cpp;
  Framework/Util.cpp;
  Framework/UtilOCV.cpp;
  Framework/UtilString.cpp;
  Framework/VideoWriter.cpp;
  Messages/ActiveUsersMessage.cpp;
  Messages/CommandMessage.cpp;
  Messages/ImageArrivedMessage.cpp;
  Messages/ImageMessage.cpp;
  Messages/ImageSizeChangedMessage.cpp;
  Messages/RoiMessage.cpp;
  Messages/UserEntriesMessage.cpp;
  Modules/ModuleConnector.cpp;
  Modules/ModuleFactory.cpp;
  Modules/FaceDetection/FaceDetection.cpp;
  Modules/FirstModule/FirstModule.cpp;
  Modules/Graph/Graph.cpp;
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
