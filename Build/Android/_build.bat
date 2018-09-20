if not exist jni mkdir jni

copy Dependencies.mk jni
copy Android.mk jni
copy Application.mk jni

cd jni
call %ANDROID_NDK%\ndk-build.cmd

xcopy ..\libs\armeabi-v7a\libface_native.so ..\..\..\Coding\AppAndroid\CameraModule\src\main\jniLibs\armeabi-v7a\ /D /Y
xcopy ..\libs\armeabi-v7a\libopencv_java3.so ..\..\..\Coding\AppAndroid\CameraModule\src\main\jniLibs\armeabi-v7a\ /D /Y

pause
