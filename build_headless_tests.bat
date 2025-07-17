@echo off
echo Building MapleStory Client with Headless Testing Support...

REM Set the path to MSBuild
set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

REM Clean previous builds
echo Cleaning previous builds...
%MSBUILD_PATH% MapleStory.vcxproj /t:Clean /p:Configuration=Debug /p:Platform=x64

REM Build the project
echo Building Debug x64 configuration...
%MSBUILD_PATH% MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo.
echo To run tests:
echo   MapleStory.exe --test                     (run all tests)
echo   MapleStory.exe --test-suite MapLoading    (run specific suite)
echo   MapleStory.exe --test-list                (list all tests)
echo   MapleStory.exe --test-help                (show help)
echo.
pause