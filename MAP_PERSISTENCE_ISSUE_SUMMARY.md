# Map Persistence Issue - Executive Summary

## Problem
Players always spawn in Henesys (map 100000000) after relogging, regardless of their logout location.

## Root Cause Analysis

### Primary Issue
The server's `MapFactory.loadMapFromWz()` method throws a `NullPointerException` when trying to load certain maps because `mapSource.getData(mapName)` returns null. This happens when:
1. The map doesn't exist in the WZ files
2. The WZ files are corrupted
3. Version mismatch between client and server WZ files

### Cascade Effect
1. `MapFactory.getMap()` returns null when map loading fails
2. `Character.loadCharFromDB()` detects null map and defaults to Henesys
3. The fallback is necessary to prevent crashes but masks the real issue

## Code Flow
```
Player Login → Load Character from DB → Get map ID → 
MapFactory.getMap(mapId) → loadMapFromWz() → getData() returns null →
NullPointerException → getMap() returns null → 
Character defaults to Henesys → Player spawns in Henesys
```

## Corrupted Map IDs Found
- 390625
- 67499489
- Potentially others above 999999999

## Proposed Fixes

### Immediate Fix (Already Applied)
```java
// In MapFactory.loadMapFromWz()
if (mapData == null) {
    log.error("Failed to load map data for map ID: {} ({})", mapid, mapName);
    return null;
}
```

### Complete Solution Needed
1. **Validate map IDs during save** - Don't save invalid map IDs to DB
2. **Add map existence check** - Verify map exists before allowing warp
3. **Improve error handling** - Log all map loading failures
4. **Fix corrupted data** - Clean up invalid map IDs in database
5. **Add fallback logic** - Use last valid town instead of always Henesys

## Investigation Tools Provided

1. **MAP_PERSISTENCE_RESEARCH_PROMPT.md** - Detailed research brief
2. **map_persistence_repomix.txt** - Relevant code sections only
3. **map_persistence_sql_investigation.sql** - Database queries
4. **map_persistence_config_files.txt** - Configuration checklist

## Next Steps for Researcher

1. **Trace a complete session**: Login → Map Change → Save → Logout → Login
2. **Identify missing maps**: Which map IDs fail to load from WZ?
3. **Find corruption source**: Why are invalid IDs being saved?
4. **Verify save timing**: Is auto-save working correctly?
5. **Test the fix**: Does the null check prevent the Henesys default?

## Success Metrics
- Players spawn at their last location
- No invalid map IDs in database
- Map loading failures are logged but handled gracefully
- No Henesys background at world select

The researcher should focus on the save/load chain and identify any additional points where map IDs might be corrupted or lost.