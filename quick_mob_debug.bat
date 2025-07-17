@echo off
echo Quick Mob Spawn Debug Test
echo =========================

REM Build the client
echo Building client...
powershell -Command "cd 'C:\HeavenClient\MapleStory-Client'; & 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe' MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64"

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo.

REM Navigate to executable directory
cd "C:\Users\me\Downloads\PERISH\MapleStory"

echo Running full pipeline test...
echo ==============================

REM Run the comprehensive pipeline test
MapleStory.exe --test-case MobSpawnDebug FullPipelineTest

echo.
echo ==============================
echo Test completed!
echo.
echo Look for these debug messages in the output:
echo [DEBUG] SpawnMobHandler::handle() called
echo [DEBUG] MapMobs::spawn() called
echo [DEBUG] MapMobs::update() called
echo.
echo If you see all three, the pipeline is working.
echo If you're missing any, that's where the issue is.
echo.
pause