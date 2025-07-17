@echo off
echo Building StatusBar number explorer...

cl /std:c++17 /I. /Iincludes explore_statusbar_numbers.cpp /link /LIBPATH:libs /LIBPATH:includes\NoLifeNx\nlnx\x64\Release NoLifeNx.lib bass.lib /out:explore_statusbar_numbers.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Running explorer...
    explore_statusbar_numbers.exe
) else (
    echo Build failed!
    pause
)