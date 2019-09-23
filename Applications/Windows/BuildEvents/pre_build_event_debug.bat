@echo off

set outDir=..\Bin\Debug
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.1.1\Windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.9.4\Windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d411d opencv_core411d opencv_features2d411d opencv_flann411d opencv_highgui411d opencv_imgcodecs411d opencv_imgproc411d opencv_ml411d opencv_objdetect411d opencv_videoio411d opencv_videoio_ffmpeg411_64)
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
