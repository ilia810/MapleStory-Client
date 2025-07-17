@echo off
cd "C:\Users\me\Downloads\PERISH\MapleStory"
timeout /t 1 /nobreak >nul
start /b MapleStory.exe
timeout /t 5 /nobreak >nul
taskkill /f /im MapleStory.exe >nul 2>&1
exit