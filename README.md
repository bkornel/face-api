# Introduction

This cross-platform face API is about adapting image processing algorithms to mobile platforms in order to achieve gesture based control in the future. The control scheme is purely depend on the user’s face, so here I focus on the adaption of my existing face recognition solutions (face detection and tracking, facial feature extraction, head pose estimation and head movement detection, see in [1], [2] and [3]) to mobile platforms (especially Android). 

An appropriate system architecture is introduced, where the user interface (UI) is entirely divided from the computer vision algorithms (hereinafter the application logic or shortly AL).

The code of AL is written in C++ and completely shared between the particular platforms. Only a thin-layer of platform-specific code are used for transferring data from/to the UI. This platform-specific code is a JNI bridge in case of our mobile platform.

A detailed description can be read in this paper [4]:<br>
https://www.researchgate.net/publication/312411829_Face_recognition_on_mobile_platforms

# Prerequsities

The Windows (Visual Studio 2017) and Android based pre-built libraries can be downloaded from the following repository: https://github.com/bkornel/3rdparty<br>
The directory structure should be the following:

```
[path_to_somewhere]
|- 3rdparty
|-- opencv-3.4.1
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

---

> [1]	K. Bertok and A. Fazekas, “Recognizing Human Activities Based on Head Movement Trajectories,” in Proc. of IEEE International Conference on CogInfoCom 2014, pp. 273 278, 2014.<br>
> [2]	K. Bertok and A. Fazekas, “Gesture Recognition - Control of a Computer with Natural Head Movements,” in Proc. of GRAPP/IVAPP 2012, pp. 527 530, 2012.<br>
> [3]	K. Bertok, L. Sajo and A. Fazekas, “A robust head pose estimation method based on POSIT algorithm,” Argumentum, vol. 7, pp. 348 356, 2011.
> [4] K. Bertok and A. Fazekas, “Face Recognition on Mobile Platforms,” In Proceedings of IEEE International Conference on CogInfoCom 2016, pp. 3743, 2016.

