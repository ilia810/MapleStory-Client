@echo off
cd "C:\Users\me\Downloads\PERISH\MapleStory"
echo Starting client...
start /b MapleStory.exe > mob_test_output.txt 2>&1
timeout /t 5 /nobreak >nul
taskkill /f /im MapleStory.exe >nul 2>&1
echo Done! Check mob_test_output.txt for results
type mob_test_output.txt | findstr /i "spawn queue"
pause