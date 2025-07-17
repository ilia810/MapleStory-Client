@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /I. /Iincludes /Iincludes\NoLifeNx\nlnx explore_maphelper.cpp /Fe:explore_maphelper.exe /link includes\NoLifeNx\nlnx\x64\Debug\NoLifeNx.lib