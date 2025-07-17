# MapleStory Stage/Map Rendering Investigation - Black Screen Issue

## Problem Update - PROGRESS MADE!

The previous **white screen transition issue is FIXED** ✅. The researcher's solution worked perfectly:
- Fixed stage clearing timing 
- Fixed graphics clear color (white → black)
- UI transition now works smoothly

## NEW ISSUE: Black Screen with Active Game State

### Current Behavior (NEW PROBLEM):
1. Character selection → smooth black fade ✅
2. Game state loads successfully ✅  
3. **Black screen persists** (but cursor visible) ❌
4. Network packets work perfectly ❌
5. NPCs/monsters spawn (packets received) ❌
6. **Map/world graphics NOT rendering** ❌

### Network Evidence (WORKING PERFECTLY):
```
[NET] Received packet opcode: 257 (SPAWN_NPC), length: 22
[NET] Received packet opcode: 259 (SPAWN_NPC_REQUEST_CONTROLLER), length: 23
[NET] Received packet opcode: 236 (SPAWN_MONSTER), length: 42
[NET] Received packet opcode: 236 (SPAWN_MONSTER), length: 42
[NET] Received packet opcode: 236 (SPAWN_MONSTER), length: 42
...
[NET] Received packet opcode: 35 (FORCED_STAT_RESET), length: 2
[NET] Received packet opcode: 63 (BUDDYLIST), length: 4
[NET] Received packet opcode: 17 (PING), length: 2
```

### Asset Loading Evidence (WORKING):
```
[NoLifeNx DEBUG] Successfully extracted bitmap using normal table (w=582, h=59)
[DEBUG FRAME] Texture created from node
[DEBUG FRAME] Creating frame from node: '17', data type: 5
[NoLifeNx DEBUG] Successfully extracted bitmap using normal table (w=572, h=59)
```

## Root Cause Analysis Needed

The issue is **NOT network or transition** - those are working perfectly. The issue is in the **map/stage rendering pipeline**.

### What's Working:
✅ Network communication (SPAWN packets received)
✅ Game state transition (cursor visible)  
✅ Asset loading (textures being created)
✅ Stage loading logic (no crashes)

### What's NOT Working:
❌ **Map background rendering**
❌ **NPC/monster visual rendering** 
❌ **Stage graphics display**
❌ **World geometry rendering**

## Investigation Focus Areas

### 1. **Stage::load() and Map Loading**
The `Stage::load(mapid, portalid)` call may be:
- Loading map data but not rendering it
- Missing map background/tileset rendering
- Viewport/camera positioning issues
- Map data corruption or missing assets

### 2. **Rendering Pipeline After Stage Load**
The rendering loop may be:
- Not calling `Stage::draw()` properly
- Graphics context issues after transition
- Missing map layer rendering calls
- Texture binding problems

### 3. **Camera/Viewport Issues**
The game view may be:
- Positioned incorrectly (looking at wrong coordinates)
- Camera not following player
- Viewport bounds issues
- Z-order rendering problems

### 4. **Map Asset Loading**
The map rendering may be failing due to:
- Missing map background assets
- Tile rendering issues  
- Layer rendering order problems
- Asset path mapping issues

## Specific Investigation Questions

1. **Is `Stage::draw()` being called?** Check if the main rendering loop calls stage drawing after transition.

2. **Is map data loaded correctly?** Verify that `Stage::load()` successfully loads map backgrounds, tiles, and geometry.

3. **Are map textures being rendered?** Check if map background and tile textures are being submitted to the graphics pipeline.

4. **Is the camera positioned correctly?** Verify player position and camera viewport after loading.

5. **Are render layers working?** Check if different map layers (background, tiles, objects) are rendering in the right order.

## Modified Transition Sequence

For reference, the current (fixed) transition code:
```cpp
Window::get().fadeout(fadestep, [mapid, portalid]() {
    // Clear old stage and graphics before loading new map
    Stage::get().clear();
    GraphicsGL::get().clear();
    Stage::get().load(mapid, portalid);
    UI::get().enable();
    Timer::get().start();
    Stage::get().transfer_player();
});
```

This sequence now works for the transition, but something in the map rendering pipeline isn't working.

## Expected Investigation Outcome

Please analyze the stage/map rendering pipeline and identify:

1. **Why map graphics aren't rendering** despite successful loading
2. **Specific fix for map/world display** 
3. **Any missing rendering calls or configuration**
4. **Camera/viewport issues** that might cause black screen

The network, transition, and basic game state are all working - we just need the map visuals to appear.

## Files Provided

`repomix_stage_rendering.txt` contains:
- `Stage.cpp` - Main stage loading and management
- `GraphicsGL.cpp` - Graphics rendering pipeline  
- `MapleMap/*` - Map rendering, backgrounds, objects
- `Graphics/Texture.cpp` - Texture management
- `Graphics/Animation.cpp` - Animation rendering
- `IO/Window.cpp` - Window and display management

Focus on the **map rendering pipeline** and **stage drawing sequence** to identify why loaded content isn't being displayed.