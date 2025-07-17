# UI Asset Loading Issues - V83 Compatibility Research

## Problem Statement
The HeavenClient (originally designed for newer MapleStory versions) is failing to load UI assets from v83 WZ files that have been converted to NX format. Despite creating a V83UIAssets compatibility layer, most UI elements are still not displaying correctly.

## Affected UI Screens
1. **Login Screen**
   - Background not loading
   - Login/Quit/New buttons missing
   - Tab buttons (MapleID/NexonID) missing
   - Fallback text "Press ENTER to login" is shown

2. **World Select Screen**
   - Background not loading
   - World/Channel buttons potentially missing

3. **Character Select Screen**
   - Background not loading
   - Character selection elements missing

4. **Character Creation Screen**
   - Race selection UI not loading (v83 only has Explorer)

5. **Skills Window**
   - Not yet updated with V83UIAssets
   - Tab structure different/missing

6. **Inventory Window**
   - Tab images missing
   - Background potentially wrong

## Current Approach
Created V83UIAssets.h compatibility layer that:
- Detects v83 mode by checking absence of UIWindow2.img
- Provides path mappings for various UI elements
- Attempts multiple naming variations for buttons
- Falls back gracefully when assets missing

## Key Technical Details
1. **WZ to NX Conversion**: v83 WZ files were converted to NX format using NoLifeWzToNx
2. **Asset Structure Differences**:
   - v83 uses UIWindow.img instead of UIWindow2.img
   - v83 uses simpler button structures without release/press states
   - v83 Login.img structure is different (uses "Title" not "Title_new")
   - v83 backgrounds often stored in Map/Obj/login.img or Map/Back/
3. **Node Types**: NX nodes can be bitmaps, containers, or have properties

## Critical Questions to Research
1. What is the actual structure of v83 Login.img in the NX files?
2. Are the button nodes named differently than expected?
3. Are backgrounds stored as bitmaps directly or in container nodes?
4. What's the correct path for each UI element in v83?
5. Why are Sprite constructors accepting nl::node but failing to display?

## Debug Information Needed
1. Complete node tree structure of Login.img
2. Node types (bitmap vs container) for each UI element
3. Actual paths to backgrounds and buttons in v83 NX files
4. Origin/offset properties that might affect positioning

## Files to Analyze
### Client Files:
- IO/UITypes/UILogin.cpp (login screen implementation)
- IO/UITypes/UIWorldSelect.cpp (world selection)
- IO/UITypes/UICharSelect.cpp (character selection)
- IO/UITypes/UIItemInventory.cpp (inventory window)
- Util/V83UIAssets.h (compatibility layer)
- Graphics/Sprite.h/cpp (sprite rendering)
- Graphics/Texture.h/cpp (texture loading)

### NX Structure to Examine:
- UI.nx/Login.img/* (all nodes and their types)
- UI.nx/UIWindow.img/* (for inventory, skills, etc.)
- Map.nx/Obj/login.img/* (for backgrounds)
- Map.nx/Back/* (alternative background location)

## Success Criteria
1. Login screen displays background and all buttons
2. World select shows world list with proper backgrounds
3. Character select displays characters with backgrounds
4. Inventory window shows with proper tabs
5. All UI elements positioned correctly

## Debugging Approach
1. Add extensive logging to show:
   - Exact node paths being accessed
   - Node types (bitmap/container/none)
   - Node children and properties
   - Sprite creation success/failure
2. Create a node tree dumper to visualize NX structure
3. Compare v83 NX structure with modern version expectations
4. Test each asset path individually