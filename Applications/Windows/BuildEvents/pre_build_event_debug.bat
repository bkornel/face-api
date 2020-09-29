@echo off

set outDir=..\Bin\Debug
set to3rdParty=..\..\..\..\3rdparty

set opencvBinDir=%to3rdParty%\opencv-4.4.0\Windows\x64\vc16\bin
set pocoBinDir=%to3rdParty%\poco-1.10.1\Windows\x64\vc16\bin
set testingDir=..\..\..\Testing

set opencvFiles=(opencv_calib3d440d opencv_core440d opencv_features2d440d opencv_flann440d opencv_highgui440d opencv_imgcodecs440d opencv_imgproc440d opencv_ml440d opencv_objdetect440d opencv_videoio440d opencv_videoio_ffmpeg440_64)
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
