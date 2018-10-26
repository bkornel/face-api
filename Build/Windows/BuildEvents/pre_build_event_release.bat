@echo off

set outDir=..\Bin\Release
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-3.4.3\Windows\x64\vc15\bin
set pocoBinDir=%to3rdParty%\poco-1.8.1\Windows\x64\vc15\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d343 opencv_core343 opencv_features2d343 opencv_ffmpeg343_64 opencv_flann343 opencv_highgui343 opencv_imgcodecs343 opencv_imgproc343 opencv_objdetect343 opencv_ml343 opencv_videoio343)
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
