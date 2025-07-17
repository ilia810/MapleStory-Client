@echo off
echo Building MapleStory Client
echo =========================

REM Set MSBuild path
set MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe

REM Check if MSBuild exists
if exist "%MSBUILD_PATH%" (
    echo Found MSBuild at: %MSBUILD_PATH%
    echo.
    echo Building solution...
    "%MSBUILD_PATH%" MapleStory.sln /p:Configuration=Debug /p:Platform=x64 /m
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo Build successful!
        echo The executable is located at: x64\Debug\MapleStory.exe
    ) else (
        echo.
        echo Build failed with error code %ERRORLEVEL%
    )
) else (
    echo MSBuild not found!
    echo Please ensure Visual Studio 2019 is installed.
)

pause