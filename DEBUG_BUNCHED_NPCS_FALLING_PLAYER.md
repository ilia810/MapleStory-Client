# Debug Guide: Bunched NPCs and Falling Player Issue

## Problem Description
- All NPCs appear bunched together in one spot
- Player is in constant falling state
- Player cannot move or interact normally
- Map appears broken or corrupted

## Root Cause Analysis

### 1. **Map Data Issues**
This problem occurs when there's a mismatch between what the server expects and what the client has loaded for map data.

### 2. **Missing Foothold Data**
- **Footholds** are invisible platforms that define where players and NPCs can stand
- Without footholds, everything falls through the map
- NPCs spawn at default positions (usually 0,0) and can't find proper ground

### 3. **Portal Problems**
- Invalid portal IDs cause spawn at (0,0)
- Missing portal data prevents proper character positioning

### 4. **Physics System Failure**
- The physics engine can't calculate proper ground positions
- Results in constant falling and no collision detection

## Debug Logging Added

I've added comprehensive debug logging to help identify the exact issue:

### Key Log Messages to Look For:

#### **Portal Issues:**
```
[Stage] Portal X returned (0,0) - portal missing or invalid!
[Stage] No portal data found for map X - spawn will fail!
[Stage] Map has 0 portals available
```

#### **Foothold Issues:**
```
[Stage] No foothold data found for map X - players will fall through!
[Stage] Found 0 footholds for map X
[Stage] Physics returned same Y position - no foothold found below spawn point!
```

#### **Map Loading Issues:**
```
[Stage] Map file path: MapX/XXXXXXXXX.img
[Stage] Failed to load map X: no WZ data found
```

#### **Position Problems:**
```
[Stage] Invalid spawn position detected: (X, Y)
[Stage] Portal X returned (0,0) - portal missing or invalid!
```

## Common Causes and Solutions

### 1. **WZ File Version Mismatch**
**Problem**: Client has v83 WZ files but server expects v87 map data (or vice versa)

**How to Detect**: Look for map loading errors:
```
[Stage] Map file path: Map1/100000000.img
[Stage] No foothold data found for map 100000000
[Stage] No portal data found for map 100000000
```

**Solution**: 
- Ensure client and server use matching WZ file versions
- Check that the map files exist in the correct directories
- Verify Map.wz contains the expected map data

### 2. **Corrupted Map Files**
**Problem**: Map.wz files are corrupted or incomplete

**How to Detect**: 
```
[Stage] Found 0 footholds for map 100000000
[Stage] Map has 0 portals available
```

**Solution**:
- Re-extract Map.wz files from clean source
- Verify file integrity
- Check if specific maps are missing

### 3. **Wrong Map ID**
**Problem**: Server sends invalid map ID that doesn't exist in client

**How to Detect**:
```
[SetFieldHandler] Invalid map ID 390625 detected (out of range)
[Stage] Map file path: Map3/390625000.img  // Invalid path
```

**Solution**:
- Use the map persistence fixes I implemented
- Check database for corrupted map IDs
- Verify server is sending valid map IDs

### 4. **Portal ID Mismatch**
**Problem**: Server sends invalid portal ID for spawn

**How to Detect**:
```
[Stage] Portal 5 returned (0,0) - portal missing or invalid!
[Stage] Attempting to use portal 0 as fallback
```

**Solution**:
- Check server-side portal ID assignments
- Verify portal data exists in map files
- Use portal 0 as fallback (should always exist)

## How to Debug Step by Step

### Step 1: Enable Debug Logging
Recompile the client with the debug logging I added and run it.

### Step 2: Check the Logs During Map Load
Look for these specific messages when the issue occurs:
1. Map file path - is it valid?
2. Foothold count - is it > 0?
3. Portal count - is it > 0?
4. Spawn position - is it valid (not 0,0)?

### Step 3: Identify the Specific Issue
Based on the logs, you'll see one of these patterns:

**Pattern A - No Map Data:**
```
[Stage] Map file path: Map1/100000000.img
[Stage] No foothold data found for map 100000000
[Stage] No portal data found for map 100000000
```
→ **Solution**: WZ file issue, map doesn't exist

**Pattern B - No Footholds:**
```
[Stage] Found 0 footholds for map 100000000
[Stage] Physics returned same Y position - no foothold found below spawn point!
```
→ **Solution**: Foothold data corrupted or missing

**Pattern C - No Portals:**
```
[Stage] Map has 0 portals available
[Stage] Portal 0 returned (0,0) - portal missing or invalid!
```
→ **Solution**: Portal data missing or corrupted

**Pattern D - Invalid Map ID:**
```
[SetFieldHandler] Invalid map ID 390625 detected (out of range)
```
→ **Solution**: Server sending wrong map ID (use my persistence fixes)

### Step 4: Apply Appropriate Fix
Based on the pattern identified, apply the corresponding solution.

## Emergency Recovery

If you need to get the client working immediately:

### 1. **Force Henesys Map**
Temporarily hardcode the map to Henesys in `SetFieldHandler`:
```cpp
// Emergency fix - force Henesys
mapid = 100000000;
portalid = 0;
```

### 2. **Check Henesys Map Data**
Verify that Henesys (100000000) has valid data:
- Map file: `Map.wz/Map/Map1/100000000.img`
- Should have foothold and portal data
- If even Henesys is broken, your WZ files are severely corrupted

### 3. **Database Cleanup**
Run this to reset all characters to Henesys:
```sql
UPDATE characters SET map = 100000000, spawnpoint = 0;
```

## Prevention

To prevent this issue from recurring:

1. **Use the map persistence fixes** I implemented
2. **Validate WZ file integrity** regularly
3. **Monitor the debug logs** for early warnings
4. **Keep client and server WZ versions synchronized**
5. **Test map changes** before deploying

## Testing Procedure

1. Recompile client with debug logging
2. Login to problematic map
3. Check debug output for the patterns above
4. Apply appropriate fix
5. Test again to verify resolution

The debug logging will show you exactly what's missing or corrupted, making it much easier to identify and fix the root cause.