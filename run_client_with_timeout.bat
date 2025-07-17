@echo off
echo Starting MapleStory client for mob spawn debugging...
echo Will auto-kill after 30 seconds
echo ===========================================

REM Navigate to executable directory
cd "C:\Users\me\Downloads\PERISH\MapleStory"

REM Start the client in background
start "MapleStory" MapleStory.exe

REM Wait 30 seconds
timeout /t 30 /nobreak

REM Kill the process
taskkill /f /im MapleStory.exe

echo Client stopped after 30 seconds
pause