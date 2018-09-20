set outDirDebug=..\Build\Windows\Bin\Debug
set outDirRelease=..\Build\Windows\Bin\Release

if not exist %outDirDebug%\configurations mkdir %outDirDebug%\configurations
xcopy configurations %outDirDebug%\configurations /E /H /C /R /Q /Y

if not exist %outDirRelease%\configurations mkdir %outDirRelease%\configurations
xcopy configurations %outDirRelease%\configurations /E /H /C /R /Q /Y

pause
