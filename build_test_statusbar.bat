@echo off
echo Building StatusBar number test...
cl /EHsc /std:c++17 /I"includes\NoLifeNx" test_statusbar_numbers.cpp /link includes\NoLifeNx\nlnx\x64\Release\NoLifeNx.lib /LIBPATH:libs /OUT:test_statusbar_numbers.exe
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo.
    echo Running test...
    test_statusbar_numbers.exe
) else (
    echo Build failed!
)