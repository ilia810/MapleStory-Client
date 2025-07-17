@echo off
echo Quick Build Test
echo ================

REM Try to find MSBuild and compile
set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
if exist "%MSBUILD_PATH%" (
    echo Using MSBuild at: %MSBUILD_PATH%
    "%MSBUILD_PATH%" MapleStory.sln /p:Configuration=Debug /p:Platform=x64
) else (
    echo MSBuild not found at expected location
    echo Please run this manually:
    echo 1. Open Visual Studio
    echo 2. Open MapleStory.sln
    echo 3. Build solution (F6)
    echo 4. Run MapleStory.exe
)

pause