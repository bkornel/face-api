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

All dependencies of the compilation in Windows (Visual Studio 2017) and Android can be downloaded (as pre-built libraries) from the following repository:<br>
https://github.com/bkornel/3rdparty<br>

You should follow the directory structure below during the compilation:
```
[local_path_of_the_project]
|- 3rdparty
|-- opencv-4.0.1
|-- ...
|- face-api
|-- Applications
|-- FaceApi
|-- Testing
|-- ...
```

# Building the API and the applications

## Windows

The solution file can be found in [Applications/Windows](https://github.com/bkornel/face-api/tree/master/Applications/Windows). Nothing else should be set up, after building the application the binaries can be found in [Applications/Windows/Bin](https://github.com/bkornel/face-api/tree/master/Applications/Windows/Bin)

## Android

The Adnroid Studio project can be found in [Applications/Android](https://github.com/bkornel/face-api/tree/master/Applications/Android). The following packages must be installed via SDK manager

### SDK platforms
- Min SDK version: API 23 (Android 6.0)
- Target SDK version: API 27 (Android 8.1)

### SDK Tools
- LLDB 3.1
- CMake 3.6.x
- NDK 19.x

The C++ part (image processing algorithms) is set up as a CMake external native build in the Android Studio project, thus it is built automatically when you make the project.

# References

> [1]	K. Bertok and A. Fazekas, "Facial gesture recognition and its applications," In Gesture recognition: performance, applications and features, Nova Science Publishers, New York, pp. 1-30, 2018.<br>
> [2]	K. Bertok and A. Fazekas, "Face recognition on mobile platforms," In Proceedings of the 7th IEEE International Conference on Cognitive Infocommunications: CogInfoCom 2016, pp. 37-43, 2016.<br>
> [3]	K. Bertok and A. Fazekas, "Recognizing complex head movements," In Australian Journal of Intelligent Information Processing Systems, vol. 14, pp. 3-17, 2016.
