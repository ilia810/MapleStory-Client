@echo off
echo Compiling mob spawn debug tool...

REM Set the path to MSBuild
set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

REM Create a temporary vcxproj file for the debug tool
echo Creating temporary project file...

(
echo ^<?xml version="1.0" encoding="utf-8"?^>
echo ^<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
echo   ^<ItemGroup Label="ProjectConfigurations"^>
echo     ^<ProjectConfiguration Include="Debug|x64"^>
echo       ^<Configuration^>Debug^</Configuration^>
echo       ^<Platform^>x64^</Platform^>
echo     ^</ProjectConfiguration^>
echo   ^</ItemGroup^>
echo   ^<PropertyGroup Label="Globals"^>
echo     ^<VCProjectVersion^>16.0^</VCProjectVersion^>
echo     ^<ProjectGuid^>{12345678-1234-1234-1234-123456789ABC}^</ProjectGuid^>
echo     ^<RootNamespace^>MobDebug^</RootNamespace^>
echo     ^<WindowsTargetPlatformVersion^>10.0^</WindowsTargetPlatformVersion^>
echo   ^</PropertyGroup^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" /^>
echo   ^<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration"^>
echo     ^<ConfigurationType^>Application^</ConfigurationType^>
echo     ^<UseDebugLibraries^>true^</UseDebugLibraries^>
echo     ^<PlatformToolset^>v142^</PlatformToolset^>
echo     ^<CharacterSet^>Unicode^</CharacterSet^>
echo   ^</PropertyGroup^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" /^>
echo   ^<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'"^>
echo     ^<IncludePath^>$(ProjectDir);$(ProjectDir)includes;$(IncludePath)^</IncludePath^>
echo     ^<LibraryPath^>$(ProjectDir)libs;$(LibraryPath)^</LibraryPath^>
echo   ^</PropertyGroup^>
echo   ^<ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'"^>
echo     ^<ClCompile^>
echo       ^<PreprocessorDefinitions^>USE_NX;WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)^</PreprocessorDefinitions^>
echo       ^<AdditionalIncludeDirectories^>$(ProjectDir);$(ProjectDir)includes;$(ProjectDir)includes\NoLifeNx;$(ProjectDir)includes\stb;$(ProjectDir)includes\freetype\include;$(ProjectDir)includes\glew-2.1.0\include;$(ProjectDir)includes\glfw-3.3.2.bin.WIN64\include;$(ProjectDir)includes\bass24\c;%(AdditionalIncludeDirectories)^</AdditionalIncludeDirectories^>
echo     ^</ClCompile^>
echo     ^<Link^>
echo       ^<AdditionalLibraryDirectories^>$(ProjectDir)libs;$(ProjectDir)includes\NoLifeNx\nlnx\x64\Debug;$(ProjectDir)includes\freetype\win64;$(ProjectDir)includes\glew-2.1.0\lib\Release\x64;$(ProjectDir)includes\glfw-3.3.2.bin.WIN64\lib-vc2019;$(ProjectDir)includes\bass24\x64;%(AdditionalLibraryDirectories)^</AdditionalLibraryDirectories^>
echo       ^<AdditionalDependencies^>opengl32.lib;glfw3.lib;glew32.lib;freetype.lib;bass.lib;NoLifeNx.lib;WzLib.lib;ws2_32.lib;kernel32.lib;user32.lib;gdi32.lib;%(AdditionalDependencies)^</AdditionalDependencies^>
echo     ^</Link^>
echo   ^</ItemDefinitionGroup^>
echo   ^<ItemGroup^>
echo     ^<ClCompile Include="debug_mob_spawn.cpp" /^>
echo   ^</ItemGroup^>
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" /^>
echo ^</Project^>
) > debug_mob_spawn.vcxproj

echo Building debug tool...
%MSBUILD_PATH% debug_mob_spawn.vcxproj /p:Configuration=Debug /p:Platform=x64

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    del debug_mob_spawn.vcxproj
    pause
    exit /b %ERRORLEVEL%
)

echo Build successful!
echo Running debug tool...
echo.

x64\Debug\debug_mob_spawn.exe

echo.
echo Debug tool completed. Cleaning up...
del debug_mob_spawn.vcxproj
if exist x64\Debug\debug_mob_spawn.exe del x64\Debug\debug_mob_spawn.exe

pause