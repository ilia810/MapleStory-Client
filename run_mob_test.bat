@echo off
cd "C:\Users\me\Downloads\PERISH\MapleStory"
echo Running mob test...
MapleStory.exe > mob_debug.txt 2>&1 &
timeout /t 5 /nobreak >nul
taskkill /f /im MapleStory.exe >nul 2>&1
echo Done! Checking for mob issues...
findstr /i "mob.constructor ERROR.*mob Creating.new.mob" mob_debug.txt
pause