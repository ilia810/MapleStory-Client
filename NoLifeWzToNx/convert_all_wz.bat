@echo off
REM convert_all_wz.bat  -- converts every .wz in .\files\ to .nx one folder up

pushd "%~dp0"                          && REM make sure we’re in NoLifeWzToNx
if not exist files\*.wz (
    echo No .wz files found in .\files\ 1>&2
    pause & exit /b 1
)

for %%F in (files\*.wz) do (
    echo Converting %%~nxF …
    NoLifeWzToNx.exe "%%F" "..\%%~nF.nx"
)

echo.
echo All done!  NX files are now in the parent folder.
popd
pause
