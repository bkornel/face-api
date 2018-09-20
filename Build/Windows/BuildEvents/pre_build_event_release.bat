@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set grtBinDir=%to3rdParty%\grt-0.2.5\Windows\x64\vc15\bin
set opencvBinDir=%to3rdParty%\opencv-3.4.1\Windows\x64\vc15\bin
set pocoBinDir=%to3rdParty%\poco-1.8.1\Windows\x64\vc15\bin
set testingDir=..\..\..\Testing

set grtFiles=(grt)
set opencvFiles=(opencv_calib3d341 opencv_core341 opencv_features2d341 opencv_ffmpeg341_64 opencv_flann341 opencv_highgui341 opencv_imgcodecs341 opencv_imgproc341 opencv_objdetect341 opencv_ml341 opencv_videoio341)
set pocoFiles=(PocoFoundation PocoUtil PocoXML PocoJSON)

for %%i in %grtFiles% do (
	xcopy %grtBinDir%\%%i.dll %outDir%  /D /Y
	xcopy %grtBinDir%\%%i.pdb %outDir%  /D /Y
)

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
