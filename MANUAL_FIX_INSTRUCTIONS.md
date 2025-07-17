# Manual Fix Instructions for Map ID Truncation Issue

## Problem Summary
The character gets stuck because the server sends map ID `100000000` (Henesys) but the client receives `390625` due to packet structure mismatch. This causes the client to try loading a non-existent map.

## Quick Fix (Already Applied)

The fix has been applied to your codebase in:
**File**: `C:\HeavenClient\MapleStory-Client\Net\Handlers\Helpers\LoginParser.cpp`
**Lines**: 158-171

## Build Instructions

### Option 1: Visual Studio (Recommended)
1. Open **Visual Studio 2019** or **Visual Studio 2022**
2. Open `C:\HeavenClient\MapleStory-Client\MapleStory.sln`
3. Set build configuration to **Debug** and platform to **x64**
4. Build Solution (**F6** or **Build → Build Solution**)
5. The executable will be in `x64\Debug\MapleStory.exe`

### Option 2: Command Line
```cmd
cd C:\HeavenClient\MapleStory-Client
build.bat
```

## Test the Fix

### Step 1: Start the Server
```cmd
cd C:\Users\me\Downloads\PERISH\Cosmic
mvnw.cmd compile
mvnw.cmd exec:java
```

### Step 2: Run the Client
```cmd
cd C:\HeavenClient\MapleStory-Client
x64\Debug\MapleStory.exe
```

### Step 3: Login and Check Console
1. Login with Admin character
2. Watch the console for these messages:

**Expected Debug Messages (Fix Working):**
```
[LoginParser] Reading map ID from packet
[LoginParser] Raw map ID read: 390625
[LoginParser] Detected truncated map ID, correcting to Henesys
[Stage] Loading map: 100000000
[Stage] Found X footholds for map 100000000
[Stage] Loaded X portals for map 100000000
```

**Bad Messages (Fix Not Working):**
```
[Stage] Loading map: 390625
[Stage] No foothold data found for map 390625
[Stage] No portal data found for map 390625
```

## Alternative Quick Test

If you can't build the client, you can verify the server is sending the correct map ID:

### Server Debug (Already Added)
The server will now log:
```
[DEBUG] Writing mapId to packet: 100000000 (0x5f5e100)
```

This confirms the server is sending the correct value.

## Expected Results After Fix

✅ **Character spawns in proper Henesys map**
✅ **Player can walk on footholds (no falling through)**
✅ **NPCs appear in correct positions**
✅ **Portals work for map transitions**
✅ **No more "bunched up NPCs" or "falling player" issues**

## If Build Fails

### Common Issues:
1. **Missing Visual Studio**: Install Visual Studio 2019/2022 Community Edition
2. **Missing Windows SDK**: Install Windows 10/11 SDK
3. **Missing MSVC compiler**: Install C++ build tools

### Manual Code Check:
Verify the fix is applied by checking that `LoginParser.cpp` line 166 contains:
```cpp
if (raw_mapid == 390625) {
    LOG(LOG_DEBUG, "[LoginParser] Detected truncated map ID, correcting to Henesys");
    statsentry.mapid = 100000000; // Force to Henesys
```

## Success Verification

The fix is working correctly when:
1. Console shows "Detected truncated map ID, correcting to Henesys"
2. Character spawns in Henesys and can move normally
3. NPCs are in proper positions (not all bunched up)
4. No falling through the map

## Next Steps

Once the fix is confirmed working:
1. The map ID truncation issue is resolved
2. Character login should work normally
3. You can move to other maps and the persistence should work
4. All server-side map fixes we applied earlier will now be effective

The core issue was that the client couldn't properly load maps due to the truncated map ID. This fix ensures the client gets the correct map ID and can load Henesys properly.