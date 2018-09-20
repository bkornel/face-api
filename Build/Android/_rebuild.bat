rmdir /S /Q obj
rmdir /S /Q libs

if not exist jni mkdir jni

del /F /Q jni\Dependencies.mk
copy Dependencies.mk jni

del /F /Q jni\Android.mk
copy Android.mk jni

del /F /Q jni\Application.mk
copy Application.mk jni

cd jni
call %ANDROID_NDK%\ndk-build.cmd

xcopy ..\libs\armeabi-v7a\libface_native.so ..\..\..\Coding\AppAndroid\CameraModule\src\main\jniLibs\armeabi-v7a\ /D /Y
xcopy ..\libs\armeabi-v7a\libopencv_java3.so ..\..\..\Coding\AppAndroid\CameraModule\src\main\jniLibs\armeabi-v7a\ /D /Y

pause
