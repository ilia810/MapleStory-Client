# Player Position Oscillation Research Request

## Critical Issue Overview

**MapleStory v83/v87 Client - Player Position Oscillating Between Extreme Values**

The player character's Y position is oscillating between +29700 and -29900 every single frame, causing severe camera shake and rendering issues. This is NOT a camera bug - the camera is correctly following an unstable player.

## Confirmed Facts

1. **Physics::get_y_below() has been fixed** - No longer returns -29901, now properly validates ground positions
2. **Camera bounds logic is working correctly** - Camera oscillation is a symptom, not the cause
3. **Root cause identified**: Player position oscillating wildly between Y=+29700 and Y=-29900 every frame
4. **Debug evidence**: 
   ```
   Line 194: Calling camera.update() with player pos: (0, 29700)
   Line 195: Calling camera.update() with player pos: (0, -29900)
   ```

## Research Objectives

**CRITICAL**: Identify why the player's physics position oscillates between extreme Y values every frame and provide a robust solution.

### Primary Investigation Areas:

1. **Player::update() Physics Loop** - Trace the player physics update cycle to find where position instability occurs
2. **FootholdTree Ground Detection** - Even though Physics::get_y_below() is fixed, investigate if foothold collision is causing player to fall through map
3. **Player State Machine** - Check if player state transitions (falling/standing) are causing position corrections that overcompensate
4. **Physics::move_object() Integration** - Analyze how player physics integrate with the foothold system
5. **Map Data Validation** - Confirm if Map 33554627 has valid foothold data that could support stable player positioning

### Secondary Investigation:

1. **Spawn Position Validation** - Verify initial player spawn position is stable
2. **Ground Collision Logic** - Check FootholdTree::limit_movement() for edge cases
3. **Player Respawn Logic** - Analyze if respawn process leaves player in unstable state

## Expected Research Deliverables

1. **Root Cause Analysis** - Precise identification of why player position oscillates
2. **Trace Analysis** - Step-by-step breakdown of the player physics update that causes instability  
3. **Robust Fix Strategy** - Multiple approaches to stabilize player position
4. **Prevention Logic** - Safeguards to prevent similar oscillation in future

## Context: Successful Fixes Already Applied

- ✅ Fixed Physics::get_y_below() with validation and fallback logic
- ✅ Added camera bounds clamping as safety net
- ✅ Fixed initial spawn position validation
- ✅ Character is now visible and rendering correctly
- ❌ **REMAINING**: Player position oscillation causing camera shake

## Key Code Locations to Investigate

1. **Player::update()** - Lines 155-190 in Player.cpp
2. **Physics::move_object()** - How player physics are processed
3. **FootholdTree::limit_movement()** - Collision resolution logic
4. **Player::respawn()** - Initial position setting
5. **PhysicsObject movement integration** - MovingObject::move() and position updates

## Debug Strategy Recommendations

1. **Add position tracking in Player::update()** to log position changes frame-by-frame
2. **Instrument FootholdTree::limit_movement()** to detect position corrections
3. **Check player's PhysicsObject state** (onground, fhid, speeds) during oscillation
4. **Validate map foothold data** for Map 33554627 specifically

## Success Criteria

- Player position stable within reasonable bounds (Y between -1000 and 2000)
- No frame-to-frame position oscillation exceeding 100 pixels
- Camera follows player smoothly without extreme jumps
- Character remains visible and properly positioned on map

This research is critical for completing the MapleStory client black screen fix. The character is now visible, but the position oscillation must be resolved for playable gameplay.