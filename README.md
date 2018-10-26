# Introduction

This cross-platform face API is about adapting image processing algorithms to mobile platforms in order to achieve gesture based control in the future. The control scheme is purely depend on the user’s face, so here I focus on the adaption of my existing face recognition solutions (face detection and tracking, facial feature extraction, head pose estimation and head movement detection, see in [1], [2] and [3]) to mobile platforms (especially to Android). 

An appropriate system architecture is introduced, where the user interface (UI) is entirely divided from the computer vision algorithms (hereinafter the application logic or shortly AL).

The code of AL is written in C++ and completely shared between the particular platforms. Only a thin-layer of platform-specific code are used for transferring data from/to the UI. This platform-specific code is a JNI bridge in case of the Android platform.

A detailed description can be read in this paper [2]:<br>
https://www.researchgate.net/publication/312411829_Face_recognition_on_mobile_platforms

You can view the results on the video below:<br>
[![Face API](http://img.youtube.com/vi/iS4eDf775GI/0.jpg)](https://www.youtube.com/watch?v=iS4eDf775GI "Face API")<br>
https://www.youtube.com/watch?v=iS4eDf775GI

# Prerequsities

The Windows (Visual Studio 2017) and Android based pre-built libraries can be downloaded from the following repository: https://github.com/bkornel/3rdparty<br>
The directory structure should be the following:

```
[path_to_somewhere]
|- 3rdparty
|-- opencv-3.4.3
|-- ...
|- face-api
|-- Build
|-- Coding
|-- ...
```

# Building the API and the applications

## Windows

The solution file can be found in [Build/Windows/Solutions](https://github.com/bkornel/face-api/tree/master/Build/Windows/Solutions). Nothing else should be set up, after building the application the binaries can be found in [Build/Windows/Bin](https://github.com/bkornel/face-api/tree/master/Build/Windows/Bin)

## Android

Install Android NDK if you have not done it yet from https://developer.android.com/ndk/downloads/

The final application is still not pushed to the repository (under development), but the AL can be built via NDK. In order to build the AL, you should call `_build.bat` in [Build/Android](https://github.com/bkornel/face-api/tree/master/Build/Android).

# References

> [1]	K. Bertok and A. Fazekas, "Facial gesture recognition and its applications," In Gesture recognition: performance, applications and features, Nova Science Publishers, New York, pp. 1-30, 2018.<br>
> [2]	K. Bertok and A. Fazekas, "Face recognition on mobile platforms," In Proceedings of the 7th IEEE International Conference on Cognitive Infocommunications: CogInfoCom 2016, pp. 37-43, 2016.<br>
> [3]	K. Bertok and A. Fazekas, "Recognizing complex head movements," In Australian Journal of Intelligent Information Processing Systems, vol. 14, pp. 3-17, 2016.
