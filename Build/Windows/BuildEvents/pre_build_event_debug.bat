@echo off

set outDir=..\Bin\Debug
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.0.1\Windows\x64\vc15\bin
set pocoBinDir=%to3rdParty%\poco-1.8.1\Windows\x64\vc15\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d401d opencv_core401d opencv_features2d401d opencv_ffmpeg401_64 opencv_flann401d opencv_highgui401d opencv_imgcodecs401d opencv_imgproc401d opencv_ml401d opencv_objdetect401d opencv_videoio401d)
set pocoFiles=(PocoFoundationd PocoUtild PocoXMLd PocoJSONd)

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
