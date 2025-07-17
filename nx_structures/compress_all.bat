@echo off
echo Compressing all NX structure files...
echo.

for %%f in (*_current.txt) do (
    if not "%%f"=="📋_NX_CURRENT_SUMMARY.txt" (
        echo Processing %%f...
        python compress_nx_structure.py "%%f"
    )
)

echo.
echo Compression complete!
pause