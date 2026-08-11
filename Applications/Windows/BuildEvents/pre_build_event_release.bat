@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.5.2\Windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.10.1\Windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d452 opencv_core452 opencv_features2d452 opencv_flann452 opencv_highgui452 opencv_imgcodecs452 opencv_imgproc452 opencv_objdetect452 opencv_ml452 opencv_videoio452 opencv_videoio_ffmpeg452_64)
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
