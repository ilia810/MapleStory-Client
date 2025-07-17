# MapleStory v83/v87 Client Camera Oscillation Analysis

## CRITICAL ISSUE IDENTIFIED: Camera Still Oscillating Despite Fixes

### Issue Summary
The camera Y position is still oscillating wildly despite the bounds checking fix being applied. The debug output shows camera Y jumping from 84 to -745 to 287 to 404 to -1201 and continuing this pattern, causing the character to be rendered outside the viewport.

### Key Debug Evidence

**From debug_all.txt:**
```
Line 171: Stage::draw - call #1 with camera view: (512, 84), viewpos: (512, 84)
Line 172: Char::draw first call - Character is being rendered!
Line 173: View position: (512, 84)
Line 174: Physics position: (0, 300)
Line 175: Absolute position: (512, 384)
Line 179: Stage::draw - call #3 with camera view: (1000, -745.128), viewpos: (1000, -745)
Line 183: Stage::draw - call #5 with camera view: (1000, 286.596), viewpos: (1000, 287)
Line 187: Stage::draw - call #7 with camera view: (1000, 403.692), viewpos: (1000, 404)
Line 190: Stage::draw - call #9 with camera view: (1000, -1200.72), viewpos: (1000, -1201)
```

**Camera Movement Pattern:**
- Frame 1: Y=84 (normal)
- Frame 3: Y=-745 (extreme negative)  
- Frame 5: Y=287 (positive)
- Frame 7: Y=404 (higher positive)
- Frame 9: Y=-1201 (extreme negative again)

### Root Cause Analysis

#### 1. **The Character Renders Initially**
- Character spawns at physics position (0, 300)
- Camera initially shows character at view position (512, 84)
- Character absolute position is (512, 384) - VISIBLE
- This proves the character data and rendering works correctly

#### 2. **Camera Oscillation Begins Immediately**
- After first frame, camera jumps to Y=-745 (off-screen)
- Camera continues oscillating between extreme negative and positive values
- X position also changes from 512 to 1000, indicating viewport or bounds issue

#### 3. **Map Bounds Issues**
```
Line 164: Portal spawn point: (0, 0)
Line 165: Physics y_below result: (0, -29901)
Line 166: WARNING: Invalid Y position -29901, forcing to Y=300
Line 167: Centering camera on player at: (0, 300)
```

The physics system is returning invalid Y positions (-29901), indicating:
- Map collision data is corrupted or missing
- Foothold system is not working properly
- This forces manual correction to Y=300

#### 4. **Render Loop Continues**
- The game continues rendering 94-96 quads consistently
- Stage::draw() is called correctly
- The issue is purely camera positioning, not rendering pipeline

### Technical Investigation Needed

#### 1. **Camera Bounds Not Being Applied**
The camera bounds fix was applied to Camera.cpp, but the oscillation suggests:
- Map bounds are not being set correctly
- The bounds values might be invalid
- The Camera::set_view() method needs investigation

#### 2. **Missing Camera Debug Output**
The camera debug output I added is not showing in the log, which means:
- The camera update method might not be called
- The debug output might not be compiled in
- Need to verify Camera::update() is actually being invoked

#### 3. **Map Data Issues**
```
Map ID: 33554627 (which is 0x2000103 in hex)
```
This map ID suggests a tutorial or starting area. The map data might be:
- Missing proper foothold information
- Has invalid wall/border definitions
- Not compatible with v83/v87 client modifications

### Immediate Actions Required

#### 1. **Debug Camera Update Calls**
Need to verify that Camera::update() is being called and with what parameters:
- Add debug output to Camera::update()
- Check if map bounds are being set correctly
- Verify the bounds calculation logic

#### 2. **Investigate Map Loading**
The map loading process needs investigation:
- Check if map 33554627 exists in the NX files
- Verify foothold data is valid
- Check if portal data is correct

#### 3. **Physics System Check**
The physics returning Y=-29901 indicates serious issues:
- Physics::get_y_below() is not finding valid ground
- Map collision detection is broken
- Need to check foothold loading and processing

### Current Status

#### What Works:
- ✅ Stage::draw() is called correctly
- ✅ Character data loads and renders
- ✅ UI state transitions work
- ✅ Rendering pipeline processes quads correctly
- ✅ Duplicate Stage::load() is prevented

#### What's Broken:
- ❌ Camera position oscillates wildly
- ❌ Map physics returns invalid positions
- ❌ Camera bounds checking not effective
- ❌ Character rendered outside viewport after first frame

### Next Steps

1. **Fix Camera Debug Output** - Ensure camera debug prints are visible
2. **Investigate Map Bounds** - Check why bounds checking isn't working
3. **Fix Physics System** - Address the -29901 Y position issue
4. **Verify Map Data** - Ensure map 33554627 has valid collision data
5. **Test with Different Map** - Try a known working map ID

### Files Requiring Investigation

1. **Camera.cpp** - Bounds checking and update logic
2. **Stage.cpp** - Map loading and physics integration
3. **Physics.cpp** - Ground detection and collision system
4. **MapInfo.cpp** - Map boundary and foothold processing
5. **Map data files** - NX file integrity for map 33554627

### Hypothesis

The camera oscillation is likely caused by:
1. Invalid map bounds being passed to Camera::set_view()
2. Camera::update() receiving incorrect position data
3. Physics system providing unstable ground positions
4. Map data corruption causing invalid collision detection

The fix applied to Camera.cpp is correct, but the underlying data being passed to the camera is invalid, causing the bounds checking to fail or produce oscillating behavior.

### Recommended Research Focus

The researcher AI should focus on:
1. Understanding why Camera::update() is causing oscillation
2. Investigating the map loading process for map 33554627
3. Analyzing the physics system's ground detection
4. Examining the relationship between map bounds and camera positioning
5. Determining if the issue is map-specific or systemic

This analysis provides the foundation for understanding why the camera continues to oscillate despite the bounds checking fix, pointing to deeper issues in the map loading and physics systems.