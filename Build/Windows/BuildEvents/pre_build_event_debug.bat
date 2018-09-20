@echo off

set outDir=..\Bin\Debug
set to3rdParty=..\..\..\..\3rdparty

set grtBinDir=%to3rdParty%\grt-0.2.5\Windows\x64\vc15\bin
set opencvBinDir=%to3rdParty%\opencv-3.4.1\Windows\x64\vc15\bin
set pocoBinDir=%to3rdParty%\poco-1.8.1\Windows\x64\vc15\bin
set testingDir=..\..\..\Testing

set grtFiles=(grtd)
set opencvFiles=(opencv_calib3d341d opencv_core341d opencv_features2d341d opencv_ffmpeg341_64 opencv_flann341d opencv_highgui341d opencv_imgcodecs341d opencv_imgproc341d opencv_ml341d opencv_objdetect341d opencv_videoio341d)
set pocoFiles=(PocoFoundationd PocoUtild PocoXMLd PocoJSONd)

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
