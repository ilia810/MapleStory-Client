# Fix for Hardcoded Henesys Issue

## Problem Found
The client was hardcoded to load Henesys (map ID 100000000) in the `Stage::draw()` method when the stage was inactive. This caused Henesys to appear:
- At world select screen
- At character select screen  
- Whenever the stage wasn't properly loaded

## Fix Applied
**File**: `C:\HeavenClient\MapleStory-Client\Gameplay\Stage.cpp`
**Line**: ~196-200

**Before**:
```cpp
if (state != State::ACTIVE) {
    static int inactive_count = 0;
    if (inactive_count++ < 10) {
        // Emergency: Force load a default map
        const_cast<Stage*>(this)->load(100000000, 0);
    }
    return;
}
```

**After**:
```cpp
if (state != State::ACTIVE) {
    // Stage is inactive - don't draw anything
    return;
}
```

## Additional Fixes Applied
1. **Server-side**: Fixed `changeMapInternal` to update `mapid` field when changing maps
2. **Server-side**: Fixed `PlayerLoggedinHandler` to call `newClient()` for all players
3. **Database**: Fixed corrupted map IDs in database

## How to Apply
1. Recompile the client with the updated Stage.cpp
2. Test by logging in - you should now spawn at your last saved location
3. The world select screen should no longer show Henesys background

## Expected Result
- Characters spawn at their last saved map location
- Map changes are properly persisted to database
- No more hardcoded Henesys forcing