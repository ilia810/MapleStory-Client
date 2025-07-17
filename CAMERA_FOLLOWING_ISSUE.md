# Camera Following Issue in MapleStory v83/v87 Client

## Problem Description

The camera system in this v83/v87 MapleStory client has two main issues:

1. **Camera fails to follow player properly** - Gets clamped at bounds when player moves to certain positions
2. **Camera shows void below the map** - The bottom bound is too permissive, allowing the viewport to extend below the actual map geometry

## Technical Context

### Coordinate System
- MapleStory uses an inverted Y coordinate system (negative Y is up, positive Y is down)
- Camera position represents the **viewport center**, not top-left corner
- When player moves right (positive X), camera X becomes negative to center on them
- Formula: `camera_x = viewport_width/2 - player_x`

### Missing VR Bounds in v83/v87
- Official MapleStory maps have VR (View Range) bounds in the map data: VRLeft, VRRight, VRTop, VRBottom
- **v83/v87 WZ/NX files are missing these VR nodes**
- The code falls back to calculating bounds from foothold (platform) data

### Current Issues

1. **Horizontal Clamping**
   - Example: Player at x=5782, camera wants x=-4623.66 but clamped to x=-4616
   - Players can move beyond the calculated bounds based on footholds
   - Current margins (5000 units) are still not enough

2. **Vertical Issues**
   - Camera shows void below the map (bottom bound too low)
   - But if we tighten bottom bound, camera can't follow player when climbing
   - Example: Player at y=-805 (high up), camera needs y=1189 but gets clamped

3. **Edge Case Examples from Debug Logs**
```
Player at (5782, 454) -> Camera clamped horizontally
Player at (-49, -355) -> Camera clamped vertically when climbing  
Player at (-1567, -1285) -> Camera correctly clamped (player very high)
```

## Current Approach

The code tries to:
1. Calculate bounds from foothold envelope + margins
2. Expand bounds if they're too small (minimum 12000 units wide, 4000 tall)
3. Add buffer in Camera.cpp (150 units) to prevent minor clipping

## Key Files

- `Gameplay/Camera.cpp` - Camera movement and bounds checking
- `Gameplay/MapleMap/MapInfo.cpp` - VR bounds calculation with v83/v87 fallback
- `Gameplay/Stage.cpp` - Sets camera bounds when loading maps
- `camera_following_issue_repomix.txt` - Contains all relevant code

## What's Needed

A better algorithm that:
1. Allows camera to follow player anywhere they can actually go
2. Minimizes void shown below/around the map
3. Handles both horizontal movement and vertical climbing gracefully
4. Works with incomplete v83/v87 map data (missing VR bounds)

The challenge is balancing permissive bounds (for following) with restrictive bounds (to hide void).