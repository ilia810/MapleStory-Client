# Build Instructions for Claude Agent

## Overview
This document contains clear instructions for building and running the MapleStory client as a Claude agent. The build process has specific requirements and steps that must be followed exactly.

## Directory Structure
- **Source Code**: `C:\HeavenClient\MapleStory-Client`
- **Execution Directory**: `C:\Users\me\Downloads\PERISH\MapleStory`
- **Settings File**: `C:\Users\me\Downloads\PERISH\MapleStory\Settings`

## Build Process

### Option 1: Using Batch Files (Recommended)
The PERISH directory contains three batch files:
- `quick_build.bat` - Incremental build, copies exe
- `build_and_run.bat` - Build and immediately run
- `build_and_test.bat` - Clean build with test instructions

To use them:
```bash
# Navigate to PERISH directory
cd "C:\Users\me\Downloads\PERISH\MapleStory"

# Run through Windows command prompt
cmd.exe /c quick_build.bat
```

### Option 2: Direct MSBuild (If batch files fail)
```bash
# Navigate to source directory
cd "C:\HeavenClient\MapleStory-Client"

# Find MSBuild (usually at one of these locations):
# VS 2019: C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe
# VS 2022: C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe

# Build command (use quotes around the entire command):
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "MapleStory.vcxproj" "/p:Configuration=Debug" "/p:Platform=x64"

# Copy executable to PERISH directory
copy "x64\Debug\MapleStory.exe" "C:\Users\me\Downloads\PERISH\MapleStory\MapleStory.exe"
```

### Option 3: Using PowerShell (RECOMMENDED - WORKS!)
```bash
# Use PowerShell through bash to build
powershell -Command "cd 'C:\HeavenClient\MapleStory-Client'; & 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe' MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64"

# The exe is automatically copied to PERISH directory by the build script
```

## Common Issues and Solutions

### Issue: MSBuild command fails with "Only one project can be specified"
**Solution**: The bash shell interprets the /p: switches incorrectly. Use cmd.exe or PowerShell, or quote parameters properly.

### Issue: "msbuild: command not found"
**Solution**: MSBuild is not in PATH. Use the full path to MSBuild.exe.

### Issue: Build succeeds but exe not found
**Solution**: Check `x64\Debug\` directory in source folder. The exe should be there after successful build.

### Issue: Game runs but Settings not loaded
**Solution**: The game must be run from the PERISH directory where the Settings file is located.

## Running the Client

1. Ensure the Settings file exists in `C:\Users\me\Downloads\PERISH\MapleStory\Settings`
2. Navigate to the PERISH directory: `cd "C:\Users\me\Downloads\PERISH\MapleStory"`
3. Run the client: `MapleStory.exe`

## Auto-Login Configuration
The Settings file should contain:
```
AutoLogin = true
AutoAccount = admin
AutoPassword = admin
AutoWorld = 0
AutoChannel = 0
AutoCharacter = 0
AutoPIC = 
```

## Debug Output
When auto-login is working, you should see:
- `[DEBUG]: [UILogin] Auto-login enabled, attempting automatic login...`
- `[DEBUG]: [UILogin] Auto-login account: admin`
- `[DEBUG]: [UILogin] Creating UILoginWait for auto-login...`
- `[DEBUG]: [UILogin] Dispatching auto-login packet...`
- `[DEBUG]: [LoginResultHandler] Login result received`
- `[DEBUG]: [LoginResultHandler] Processing login result with active UILoginWait`

## Quick Test Command Sequence
```bash
# Build and copy
cd "C:\Users\me\Downloads\PERISH\MapleStory"
cmd.exe /c quick_build.bat

# Run
cd "C:\Users\me\Downloads\PERISH\MapleStory"
MapleStory.exe
```