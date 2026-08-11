# Introduction

This cross-platform face API is about achieving facial information driven development (e.g. gesture-based control) in mobile environment. The scheme is purely based on the user's face or more precisely on the following data:

- Face rectangle in the 2-D pixel-, and the face box in the 3-D camera coordinate system
- 66 pieces of facial feature points in the 2-D pixel-, and 3-D camera coordinate system
- 6DoF head pose (to the camera) in the 3-D camera coordinate system

All the information is determined on monocular images therefore there is no need for a special hardware or sensor during the calculations. If you are interested in the technical background then feel free to read in the following publications: [1], [2] and [3]

The system architecture is designed to reuse the image processing algorithms in case of multiple host platforms. The code of application logic (AL) is written in C++ and is shared between the particular platforms. Only a thin-layer of platform-specific code are used for transferring data from/to the user interface (UI). For example, this platform-specific code is a JNI bridge in case of the Android host platform.

A detailed description about the architecture can be read in this paper [2]:<br>
https://www.researchgate.net/publication/312411829_Face_recognition_on_mobile_platforms

You can view the results on the video below:<br>
[![Face API](http://img.youtube.com/vi/iS4eDf775GI/0.jpg)](https://www.youtube.com/watch?v=iS4eDf775GI "Face API")<br>
https://www.youtube.com/watch?v=iS4eDf775GI

# Prerequisites

All dependencies of the compilation in Windows (Visual Studio 2022) and Android can be downloaded (as pre-built libraries) from the following repository:<br>
https://github.com/bkornel/3rdparty<br>

The API is built against OpenCV 4.5.2, and the Windows application also uses Poco 1.10.1.

You should follow the directory structure below during the compilation:
```
[local_path_of_the_project]
|- 3rdparty
|-- opencv-4.5.2
|-- poco-1.10.1
|-- ...
|- face-api
|-- Applications
|-- FaceApi
|-- Testing
|-- ...
```

# Building the API and the Applications

## Windows

The solution file can be found in [Applications/Windows](https://github.com/bkornel/face-api/tree/master/Applications/Windows). Nothing else should be set up, after building the application the binaries can be found in [Applications/Windows/Bin](https://github.com/bkornel/face-api/tree/master/Applications/Windows/Bin)

A pre-build step ([`Deploy-Dependencies.ps1`](https://github.com/bkornel/face-api/blob/master/Applications/Windows/BuildEvents/Deploy-Dependencies.ps1)) copies the OpenCV and Poco binaries and the test configurations next to the executable, so the application can be started right after building. A missing dependency fails the build instead of producing an executable that cannot start.

## Android

The Android Studio project can be found in [Applications/Android](https://github.com/bkornel/face-api/tree/master/Applications/Android). The following packages must be installed via SDK manager

### SDK platforms
- Min SDK version: API 24 (Android 7.0)
- Compile and target SDK version: API 35 (Android 15)

### SDK Tools
- CMake 3.22.x or newer
- NDK 26.x or newer
- JDK 17 (bundled with recent Android Studio)

The project builds with Android Gradle plugin 8.5 and Gradle 8.7, and uses AndroidX.

The C++ part (image processing algorithms) is set up as a CMake external native build in the Android Studio project, thus it is built automatically when you make the project. OpenCV is linked statically, so no OpenCV shared object has to be packaged into the APK.

Both `armeabi-v7a` and `arm64-v8a` are built. The OpenCV directory name is taken from the `OPENCV_VERSION` CMake cache variable, which defaults to `opencv-4.5.2`; override it if you keep a different version under `3rdparty`.

The app stores its working files (settings, cascade files, shape models, logs) in app private storage, so it needs no storage permission. Only `CAMERA` is requested at runtime.

# Modules

The whole module graph can be created from the settings file (defined in [`settings.json`](https://github.com/bkornel/face-api/blob/master/Testing/configurations/settings.json) by default). Modules can interact and exchange information whith each others via ports. Every module must have one output port and can have any input ports (zero or more).

Ports can be defined by implementing the `fw::Port` interface. For example the [`UserManager`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/UserManager/UserManager.h) class:

```
class UserManager :
  public fw::Module,
  public fw::Port<ActiveUsersMessage::Shared(ImageMessage::Shared, RoiMessage::Shared)>
{
  ...
  ActiveUsersMessage::Shared Main(ImageMessage::Shared iImage, RoiMessage::Shared iDetections) override;
  ...
};
```

This class has the `ActiveUsersMessage::Shared` output and the `ImageMessage::Shared` and `RoiMessage::Shared` inputs. The `Main` member function of the class must defined according to the template arguments of `fw::Port`.

A module that declares no input port at all is a source: it has nothing to wait for, so it is driven by `Trigger()` instead of by a predecessor. This is how [`FirstModule`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FirstModule/FirstModule.h) starts every frame. Input ports are optional as well: a port that is left out of the settings file is not counted as a dependency, and its argument reaches `Main` default-constructed. A module with declared inputs of which none is connected is rejected, because nothing would ever make it run.

## Module Graph

The module graph can be defined in the settings file ([`settings.json`](https://github.com/bkornel/face-api/blob/master/Testing/configurations/settings.json) by default). For the [`UserManager`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/UserManager/UserManager.h) module it is:

```
"userManager": {
  "port": [ "imageQueue:1", "faceDetection:2" ],
  ...
}
```

Where the [`ImageQueue`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ImageQueue/ImageQueue.h) module returns with an `ImageMessage::Shared` and transfers the information to the first inputport of [`UserManager`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/UserManager/UserManager.h) and so does the [`FaceDetection`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FaceDetection/FaceDetection.h) with `RoiMessage::Shared`.

## Getting the Results

There are two ways to read what the API determined. `FaceApi::GetResultImage` returns the frame the [`Visualizer`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/Visualizer/Visualizer.h) drew the overlay on, which is the simplest way to display something and what the Windows application uses. `FaceApi::GetResults` returns the same information as data instead — the face rectangle, the feature points in 2-D and 3-D, the face box and the head pose — so the host application can draw it with its own canvas or GPU surface. For hosts that cannot take C++ structures across their language boundary, `face::result_buffer` packs the results into a flat float buffer; the Android application reads that from its JNI bridge.

Drawing on the host side is the cheaper path, because the frame is neither composited nor copied back, which matters on a mobile device. It is switched on by leaving the `visualizer` module out of the settings file, in which case `lastModule` takes the frame from the queue directly:

```
"lastModule": {
  "port": [ "imageQueue:1", "userProcessor:2" ]
}
```

The second port carries the users and is the one `GetResults` reports. It is optional, so a graph that only needs the rendered frame can leave it out.

The Android application is set up this way. `Native.getResults` fills a reused `float[]` of a few kilobytes, `FaceOverlayRenderer` draws the landmarks, the face box and a label from it, and `FaceOverlayView` puts that on a transparent view above the live camera preview. Only when a photo is captured does the native side render a frame at all.

## Adding a New Module

The native side must be only extended in case of adding a new module to the system.
- First the `ModuleFactory::Create(...)` in [`ModuleFactory.cpp`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ModuleFactory.cpp) which creates and initializes every module
- Then the `ModuleConnector::Connect(...)` in [`ModuleConnector.cpp`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ModuleConnector.cpp) which connects all the inputs (predecessors) to a given module
- Finally the `connect(...)` in the anonymous namespace of [`ModuleConnector.cpp`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ModuleConnector.cpp) which connects the ith input to a given module

These are just copy-pasting 1-2 line, you should find the `// REMARK: Insert new modules here` comments in the files above.

Beside these, the `Main` of a new module should start with a `DrainCommands()` call. Commands, for example that the image size has changed, are published on a message bus by the thread that raises them, and `DrainCommands` applies them on the thread of the module graph. This is what keeps a module's own state touched from one thread only.

# References

> [1]	K. Bertok and A. Fazekas, "Facial gesture recognition and its applications," In Gesture recognition: performance, applications and features, Nova Science Publishers, New York, pp. 1-30, 2018.<br>
> [2]	K. Bertok and A. Fazekas, "Face recognition on mobile platforms," In Proceedings of the 7th IEEE International Conference on Cognitive Infocommunications: CogInfoCom 2016, pp. 37-43, 2016.<br>
> [3]	K. Bertok and A. Fazekas, "Recognizing complex head movements," In Australian Journal of Intelligent Information Processing Systems, vol. 14, pp. 3-17, 2016.
