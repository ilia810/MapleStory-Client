# MapleStory v83/v87 Map Persistence Issue - Research Task

## Problem Statement
Players are experiencing a critical issue where they always spawn in Henesys (map ID 100000000) after logging in, regardless of where they logged out. Map changes appear to work during gameplay, but the location is not persisting between sessions.

## Current Symptoms
1. Player changes maps successfully during gameplay
2. Upon relogging, player always spawns in Henesys
3. The Henesys map background is visible even at world select screen (before character login)
4. Database shows some corrupted map IDs (e.g., 390625, 67499489)

## Technical Context
- **Server**: Cosmic v83 source (Java-based)
- **Client**: HeavenClient (C++ based)
- **Database**: MySQL
- **Version**: v83/v87 hybrid compatibility

## What We've Discovered So Far

### Server-Side Findings:
1. **MapFactory.java** has a known issue where `mapSource.getData(mapName)` can return null, causing a NullPointerException
2. **Character.java** has fallback logic that defaults to Henesys when a map fails to load
3. The save logic appears correct - it updates the mapid field during map changes
4. Some maps have `forcedReturn` values that might interfere with persistence

### Client-Side Findings:
1. **Stage.cpp** loads maps correctly but may have initialization issues
2. **SetFieldHandler.cpp** handles the SET_FIELD packet from server and includes validation for corrupted map IDs
3. The client properly sends map change requests to the server

### Database Findings:
1. Default map value in the schema is 0 (not 100000000)
2. Some characters have invalid/corrupted map IDs stored

## Key Code Flow

### Login Flow:
1. Client connects → Server loads character from DB
2. Server tries to load map via MapFactory.getMap(mapId)
3. If map data is null (WZ file missing/corrupted), server defaults to Henesys
4. Client receives SET_FIELD packet with the map ID
5. Client loads the map and displays it

### Map Change Flow:
1. Player enters portal → Client sends ChangeMapPacket
2. Server processes change, updates character's map
3. Server should save to DB (but timing/success unclear)
4. Client receives new map data and transitions

## Research Objectives

1. **Identify the exact point of failure**: Why are map IDs not persisting correctly in the database?

2. **Find the corruption source**: What's causing invalid map IDs (390625, 67499489) to be saved?

3. **Trace the save flow**: Verify that `saveCharToDB` is actually being called after map changes and that the correct map ID is being written

4. **Investigate initialization**: Why does Henesys appear at world select? Is there hardcoded initialization?

5. **Validate the fix approach**: Is the null check in MapFactory sufficient, or are there other edge cases?

## Specific Questions to Answer

1. When exactly is `Character.saveCharToDB()` called? Is it called after every map change or only periodically?

2. Are there any server events or conditions that might reset a character's map to Henesys?

3. What's the relationship between `mapid` field and `map` object in Character.java? Are they always synchronized?

4. Are there any login-time map validations that might override the saved map ID?

5. Is there a race condition between map changes and auto-save that could cause the old map to be saved?

## Suggested Investigation Approach

1. Add comprehensive logging throughout the map save/load chain
2. Trace a single character through login → map change → logout → login
3. Check for any background threads or scheduled tasks that might affect map persistence
4. Verify WZ file integrity for common maps
5. Look for any hardcoded map assignments during character creation or login

## Success Criteria

A fix is successful when:
1. Players spawn at their last location after relogging
2. No corruption of map IDs in the database
3. Invalid maps gracefully fallback without losing the intended destination
4. The Henesys background doesn't appear during world select

## Files Included in Repomix

The attached repomix contains the most relevant files for investigating this issue, including:
- Server-side: Character save/load logic, map handling, login handlers
- Client-side: Map loading, SET_FIELD packet handling, stage management
- Database: Schema definitions and sample queries
- Configuration: Example map IDs and constants

Please analyze the code flow and identify the root cause of the map persistence failure.