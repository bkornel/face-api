@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.0.1\Windows\x64\vc15\bin
set pocoBinDir=%to3rdParty%\poco-1.8.1\Windows\x64\vc15\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d401 opencv_core401 opencv_features2d401 opencv_ffmpeg401_64 opencv_flann401 opencv_highgui401 opencv_imgcodecs401 opencv_imgproc401 opencv_objdetect401 opencv_ml401 opencv_videoio401)
set pocoFiles=(PocoFoundation PocoUtil PocoXML PocoJSON)

for %%i in %pocoFiles% do (
	xcopy %pocoBinDir%\%%i.dll %outDir%  /D /Y
	xcopy %pocoBinDir%\%%i.pdb %outDir%  /D /Y
)

for %%i in %opencvFiles% do (
	xcopy %opencvBinDir%\%%i.dll %outDir%  /D /Y
	xcopy %opencvBinDir%\%%i.pdb %outDir%  /D /Y
)

if not exist %outDir%\configurations mkdir %outDir%\configurations
xcopy %testingDir%\configurations %outDir%\configurations /E /H /C /R /Q /Y
