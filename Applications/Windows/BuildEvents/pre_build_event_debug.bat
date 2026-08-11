@echo off

set outDir=..\Bin\Debug
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.5.2\Windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.10.1\Windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d452d opencv_core452d opencv_features2d452d opencv_flann452d opencv_highgui452d opencv_imgcodecs452d opencv_imgproc452d opencv_ml452d opencv_objdetect452d opencv_videoio452d opencv_videoio_ffmpeg452_64)
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
