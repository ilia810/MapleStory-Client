@echo off
echo Building NX Explorer...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /EHsc explore_portal_structure.cpp /I includes/NoLifeNx/nlnx /I includes/NoLifeNx/nlnx/includes/lz4_v1_8_2_win64/include includes/NoLifeNx/nlnx/x64/Debug/NoLifeNx.lib
echo Done!