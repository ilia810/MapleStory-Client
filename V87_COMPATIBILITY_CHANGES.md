# HeavenClient v87 Compatibility Changes

This document summarizes the changes made to enable HeavenClient to work with original MapleStory v87 WZ files.

## Overview

The original HeavenClient was designed for modern MapleStory data (v228+) and expected specific UI structures and assets that don't exist in v87. These changes implement fallback logic and compatibility layers to handle the older data format.

## Key Changes Made

### 1. NxFiles.cpp - Removed Strict Version Checking
**File**: `/mnt/c/HeavenClient/MapleStory-Client/Util/NxFiles.cpp`

- **Removed**: Strict UI version check that looked for "Login.img/WorldSelect/BtChannel/layer:bg"
- **Added**: Flexible file loading that only requires essential files (Base.nx, Character.nx, etc.)
- **Added**: Optional file checking with warnings instead of errors for missing v87-era files

**Impact**: Client no longer crashes with "WRONG_UI_FILE" error when using v87 data.

### 2. UIWorldSelect.cpp - Major UI Compatibility Updates
**File**: `/mnt/c/HeavenClient/MapleStory-Client/IO/UITypes/UIWorldSelect.cpp`

#### World Selection Structure
- **Added**: Fallback logic for missing modern UI structure (`BtWorld.release`, `BtChannel`)
- **Added**: v87-compatible world button creation using direct indexing (world0, world1, etc.)
- **Added**: Default positioning for v87 worlds when region data is missing

#### Channel Selection 
- **Added**: Auto-entry to channel 1 when modern channel UI is missing
- **Modified**: Button creation to check for asset existence before instantiation
- **Added**: Safe drawing of textures with empty-check guards

#### Background Loading
- **Added**: Fallback from Map data to UI data for login backgrounds
- **Added**: Graceful handling of missing background assets
- **Modified**: World mapping to use v87-appropriate worlds (no Reboot)

**Impact**: World selection works with v87's simpler UI structure, automatically entering worlds without complex channel selection.

### 3. UIChatBar.cpp - Chat UI Button Safety
**File**: `/mnt/c/HeavenClient/MapleStory-Client/IO/UITypes/UIChatBar.cpp`

- **Added**: Conditional button creation for post-v87 features (itemLink, chatEmoticon)
- **Added**: Dynamic button positioning to accommodate missing buttons
- **Modified**: Button existence checks before instantiation

**Impact**: Chat UI loads successfully even when v87 data lacks modern chat features.

## v87 Compatibility Features

### Automatic Fallbacks
1. **World Selection**: If modern channel UI missing → auto-enter channel 1
2. **Background Images**: If Map data missing → try UI data → continue without background
3. **UI Buttons**: If button assets missing → skip creation, continue without feature
4. **File Loading**: If optional files missing → log warning, continue loading

### Safe Drawing
- All texture drawing now checks for empty textures before attempting to draw
- Missing UI elements are gracefully skipped rather than causing crashes
- Bounds checking added for arrays and vectors

### v87-Specific Adaptations
- World list updated to v87-era worlds (Scania, Bera, Broa, Windia, Khaini)
- Removed dependencies on Reboot world features
- Simplified region handling for single-region v87 clients

## Files Modified

1. `/mnt/c/HeavenClient/MapleStory-Client/Util/NxFiles.cpp` - File loading and version checking
2. `/mnt/c/HeavenClient/MapleStory-Client/IO/UITypes/UIWorldSelect.cpp` - World/channel selection UI
3. `/mnt/c/HeavenClient/MapleStory-Client/IO/UITypes/UIChatBar.cpp` - Chat UI buttons

## Technical Implementation

### Error Handling Strategy
- **Before**: Strict validation, crash on missing assets
- **After**: Graceful degradation, continue with available assets

### UI Loading Strategy  
- **Before**: Expect specific modern UI paths
- **After**: Try modern paths first, fallback to v87 paths, skip if neither exists

### Asset Management
- **Before**: Assume all UI assets exist
- **After**: Check asset existence, provide defaults or skip features

## Testing Recommendations

When testing with v87 data:

1. **Login Flow**: Verify login screen appears without background errors
2. **World Selection**: Test world button functionality and auto-channel entry
3. **Chat Interface**: Confirm chat works without item link/emoticon buttons
4. **File Loading**: Check that only essential missing files cause errors

## Limitations

These changes enable basic functionality with v87 data but some features remain incompatible:

- **Modern UI Features**: Item linking, chat emoticons unavailable in v87
- **Post-v87 Content**: Reboot worlds, newer job classes not supported
- **Advanced Channel UI**: No channel population display or selection in v87 mode

## Future Enhancements

Potential improvements for better v87 support:

1. **Version Detection**: Auto-detect v87 vs modern data and adjust behavior
2. **Configuration**: Allow users to specify legacy mode for full v87 optimization  
3. **UI Reconstruction**: Create simplified v87-style UIs for missing elements
4. **Content Filtering**: Hide or disable post-v87 game features automatically

## Conclusion

These changes successfully enable HeavenClient to initialize and run with original v87 WZ files while maintaining compatibility with modern data. The client gracefully handles missing assets and provides a functional gaming experience appropriate to the v87 era.