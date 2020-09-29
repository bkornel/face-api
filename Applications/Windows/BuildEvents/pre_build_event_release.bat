@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.4.0\windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.10.1\windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d440 opencv_core440 opencv_features2d440 opencv_flann440 opencv_highgui440 opencv_imgcodecs440 opencv_imgproc440 opencv_objdetect440 opencv_ml440 opencv_videoio440 opencv_videoio_ffmpeg440_64)
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
