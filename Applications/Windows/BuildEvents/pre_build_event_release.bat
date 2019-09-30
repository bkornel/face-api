@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.1.1\Windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.9.4\Windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d411 opencv_core411 opencv_features2d411 opencv_flann411 opencv_highgui411 opencv_imgcodecs411 opencv_imgproc411 opencv_objdetect411 opencv_ml411 opencv_videoio411 opencv_videoio_ffmpeg411_64)
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
