# MapleStory v83/v87 Client Black Screen Issue Analysis

## Issue Summary
After logging in and transitioning to the game state, the character appears briefly (seen twice during fade in/out), but then the screen goes completely black despite the render loop continuing normally.

## Debug Evidence

### Key Observations from debug_all.txt:

1. **Before Login (frames 1-100)**: 17-18 quads drawn (login UI elements)
2. **After Login (frames 100+)**: 94-96 quads being drawn
3. **Character visibility**: Character renders correctly during fade transitions
4. **Post-fade issue**: Screen goes black after fade completes despite 94-96 quads still being drawn

### Character Position Debug:
```
[STATETRANS] Char::draw first call - Character is being rendered!
[STATETRANS] View position: (512, 84)
[STATETRANS] Physics position: (0, 300)
[STATETRANS] Absolute position: (512, 384)
```

### Camera Movement Pattern:
The camera view is oscillating wildly between different Y positions:
- Frame 1: (512, 84)
- Frame 100: (512, 491.298)
- Frame 200: (512, 221.73)
- Frame 300: (512, 413.754)
- Frame 2400: (512, 496.374)

## Root Cause Analysis

### 1. **Window Fade Mechanism**
- During fadeout, opacity reaches nearly 0 (3.91155e-07)
- Black overlay with alpha = 1.0 would make everything black
- Fixed by increasing threshold from 0.01 to 0.1

### 2. **Camera Positioning Issue** 
- Camera is not stable - it's constantly moving/oscillating
- Player is at position (0, 300)
- Camera should center on player but is moving erratically

### 3. **Viewport Clipping**
- Character might be rendered outside the visible viewport
- Camera view calculations may be incorrect

### 4. **Render Pipeline**
The render flow is:
```
Main draw() -> Window::begin() -> clearscene() -> Stage::draw() -> UI::draw() -> Window::end() -> flush()
```

## Current State of Fixes Applied

### 1. **UIStateGame::draw()** - FIXED
Added missing Stage::draw() call that was causing initial black screen

### 2. **Stage::load() duplicate call** - FIXED
Added safeguard to prevent duplicate loading when Stage is already ACTIVE

### 3. **Character spawn position** - FIXED
Fixed invalid Y position from -29901 to 300

### 4. **Fade opacity threshold** - PARTIALLY FIXED
Increased from 0.01 to 0.1 to prevent near-black overlay

### 5. **Camera positioning** - ATTEMPTED FIX
Changed from hardcoded position to centering on player, but camera is still unstable

## The Real Issue

Looking at the debug output, the camera view Y coordinate is oscillating wildly:
- It moves from 84 to 550 to 180 to 560 etc.
- This unstable camera is likely causing the character to be rendered outside the viewport
- The 94-96 quads are being drawn but may be off-screen

## Suspected Root Causes

1. **Camera update logic in Camera::update()** is causing oscillation
2. **Map bounds checking** might be incorrect (see the "Map borders too large" warning)
3. **Physics/collision detection** might be affecting camera position
4. **View calculations** in Stage::draw() might have errors

## Next Steps for Investigation

1. **Stabilize the camera**: The camera Y position should not oscillate wildly
2. **Check map bounds**: The map boundaries might be causing camera issues
3. **Verify viewport calculations**: Ensure character is drawn within visible area
4. **Debug quad positions**: Check if the 94-96 quads are actually on-screen

## Files Involved

1. `Graphics/GraphicsGL.cpp` - Handles rendering and fade effects
2. `IO/UIStateGame.cpp` - Game state UI management
3. `Gameplay/Stage.cpp` - Main game stage/world rendering
4. `IO/Window.cpp` - Window management and fade transitions
5. `Character/Char.cpp` - Character rendering
6. `Gameplay/Camera.cpp` - Camera positioning and movement
7. `Net/Handlers/SetFieldHandlers.cpp` - Network handler for map transitions

## Repomix Output
See `repomix_black_screen_issue.txt` for complete source code of all relevant files.

## Current Working Theory
The black screen is caused by the camera oscillating to positions where the character and game world are outside the visible viewport. While 94-96 quads are being rendered, they're likely positioned off-screen due to incorrect camera calculations.