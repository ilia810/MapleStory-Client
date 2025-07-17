@echo off
echo Running Mob Spawn Debug Tests with Headless Framework
echo =====================================================

REM Build the client with test support
echo Building client...
powershell -Command "cd 'C:\HeavenClient\MapleStory-Client'; & 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe' MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64"

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo.

REM Navigate to the executable directory
cd "C:\Users\me\Downloads\PERISH\MapleStory"

echo Running mob spawn debug tests...
echo.

REM Run all mob spawn debug tests
echo === Running All MobSpawnDebug Tests ===
MapleStory.exe --test-suite MobSpawnDebug

echo.
echo === Running Individual Tests ===

echo.
echo 1. Connection Status Test
MapleStory.exe --test-case MobSpawnDebug ConnectionStatus

echo.
echo 2. Map Loaded Status Test  
MapleStory.exe --test-case MobSpawnDebug MapLoadedStatus

echo.
echo 3. Manual Spawn Test
MapleStory.exe --test-case MobSpawnDebug ManualSpawnTest

echo.
echo 4. Packet Handler Test
MapleStory.exe --test-case MobSpawnDebug PacketHandlerTest

echo.
echo 5. Update Loop Test
MapleStory.exe --test-case MobSpawnDebug UpdateLoopTest

echo.
echo 6. Server Packet Monitor
MapleStory.exe --test-case MobSpawnDebug ServerPacketMonitor

echo.
echo 7. Full Pipeline Test
MapleStory.exe --test-case MobSpawnDebug FullPipelineTest

echo.
echo =====================================================
echo Mob spawn debug tests completed!
echo.
echo Check the output above for debug messages showing:
echo - [DEBUG] SpawnMobHandler::handle() called
echo - [DEBUG] MapMobs::spawn() called  
echo - [DEBUG] MapMobs::update() called
echo.
echo This will show exactly where the mob spawning pipeline breaks.
echo.
pause