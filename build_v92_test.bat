@echo off
echo Building v92 detection test...

set MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
set VCPKG_DIR=C:\vcpkg
set PROJECT_DIR=C:\HeavenClient\MapleStory-Client

:: Create a simple test project
echo ^<?xml version="1.0" encoding="utf-8"?^> > test_v92.vcxproj
echo ^<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^> >> test_v92.vcxproj
echo   ^<ItemGroup Label="ProjectConfigurations"^> >> test_v92.vcxproj
echo     ^<ProjectConfiguration Include="Debug|x64"^> >> test_v92.vcxproj
echo       ^<Configuration^>Debug^</Configuration^> >> test_v92.vcxproj
echo       ^<Platform^>x64^</Platform^> >> test_v92.vcxproj
echo     ^</ProjectConfiguration^> >> test_v92.vcxproj
echo   ^</ItemGroup^> >> test_v92.vcxproj
echo   ^<PropertyGroup Label="Globals"^> >> test_v92.vcxproj
echo     ^<VCProjectVersion^>16.0^</VCProjectVersion^> >> test_v92.vcxproj
echo     ^<ProjectGuid^>{B0520317-1234-5678-9ABC-DEF012345678}^</ProjectGuid^> >> test_v92.vcxproj
echo     ^<RootNamespace^>test_v92^</RootNamespace^> >> test_v92.vcxproj
echo     ^<WindowsTargetPlatformVersion^>10.0^</WindowsTargetPlatformVersion^> >> test_v92.vcxproj
echo   ^</PropertyGroup^> >> test_v92.vcxproj
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" /^> >> test_v92.vcxproj
echo   ^<PropertyGroup Label="Configuration"^> >> test_v92.vcxproj
echo     ^<ConfigurationType^>Application^</ConfigurationType^> >> test_v92.vcxproj
echo     ^<UseDebugLibraries^>true^</UseDebugLibraries^> >> test_v92.vcxproj
echo     ^<PlatformToolset^>v142^</PlatformToolset^> >> test_v92.vcxproj
echo     ^<CharacterSet^>MultiByte^</CharacterSet^> >> test_v92.vcxproj
echo   ^</PropertyGroup^> >> test_v92.vcxproj
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" /^> >> test_v92.vcxproj
echo   ^<PropertyGroup^> >> test_v92.vcxproj
echo     ^<IncludePath^>$(ProjectDir);$(ProjectDir)includes;%VCPKG_DIR%\installed\x64-windows\include;$(IncludePath)^</IncludePath^> >> test_v92.vcxproj
echo     ^<LibraryPath^>$(ProjectDir)includes\NoLifeNx\nlnx\x64\Debug;%VCPKG_DIR%\installed\x64-windows\lib;$(LibraryPath)^</LibraryPath^> >> test_v92.vcxproj
echo   ^</PropertyGroup^> >> test_v92.vcxproj
echo   ^<ItemDefinitionGroup^> >> test_v92.vcxproj
echo     ^<ClCompile^> >> test_v92.vcxproj
echo       ^<WarningLevel^>Level3^</WarningLevel^> >> test_v92.vcxproj
echo       ^<Optimization^>Disabled^</Optimization^> >> test_v92.vcxproj
echo       ^<SDLCheck^>false^</SDLCheck^> >> test_v92.vcxproj
echo       ^<PreprocessorDefinitions^>USE_NX;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)^</PreprocessorDefinitions^> >> test_v92.vcxproj
echo       ^<LanguageStandard^>stdcpp17^</LanguageStandard^> >> test_v92.vcxproj
echo       ^<RuntimeLibrary^>MultiThreadedDebugDLL^</RuntimeLibrary^> >> test_v92.vcxproj
echo     ^</ClCompile^> >> test_v92.vcxproj
echo     ^<Link^> >> test_v92.vcxproj
echo       ^<SubSystem^>Console^</SubSystem^> >> test_v92.vcxproj
echo       ^<AdditionalDependencies^>NoLifeNx.lib;lz4d.lib;%(AdditionalDependencies)^</AdditionalDependencies^> >> test_v92.vcxproj
echo     ^</Link^> >> test_v92.vcxproj
echo   ^</ItemDefinitionGroup^> >> test_v92.vcxproj
echo   ^<ItemGroup^> >> test_v92.vcxproj
echo     ^<ClCompile Include="test_v92_detection.cpp" /^> >> test_v92.vcxproj
echo     ^<ClCompile Include="Util\NxFiles.cpp" /^> >> test_v92.vcxproj
echo     ^<ClCompile Include="Util\Misc.cpp" /^> >> test_v92.vcxproj
echo     ^<ClCompile Include="Configuration.cpp" /^> >> test_v92.vcxproj
echo   ^</ItemGroup^> >> test_v92.vcxproj
echo   ^<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" /^> >> test_v92.vcxproj
echo ^</Project^> >> test_v92.vcxproj

:: Build the test
%MSBUILD% test_v92.vcxproj /p:Configuration=Debug /p:Platform=x64

if exist x64\Debug\test_v92.exe (
    echo.
    echo Running test...
    cd x64\Debug
    test_v92.exe
    cd ..\..
) else (
    echo Build failed!
)

:: Cleanup
del test_v92.vcxproj
pause