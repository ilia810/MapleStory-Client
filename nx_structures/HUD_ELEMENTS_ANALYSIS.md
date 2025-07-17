# MapleStory HUD Elements Analysis

## Summary of HUD (Heads-Up Display) Elements

### 1. UIStatusBar (HP/MP/EXP bars)
- **File**: `IO/UITypes/UIStatusBar.cpp`
- **Primary Asset Path**: `UI/StatusBar3.img/mainBar`
- **Sub-assets**:
  - `mainBar/status` (or `status800` for 800px width)
  - `mainBar/EXPBar`
  - `mainBar/menu`
  - `mainBar/quickSlot`
  - `mainBar/submenu`
- **Components**:
  - HP/MP bars with gauges
  - Experience bar
  - Level display
  - Player name label
  - Menu buttons (Cash Shop, Menu, Options, Character, Community, Event)
  - Quick slot buttons

### 2. UIKeyConfig (Hotkeys/Quickslots)
- **File**: `IO/UITypes/UIKeyConfig.cpp`
- **Primary Asset Path**: `UI/StatusBar3.img/KeyConfig`
- **Sub-assets**:
  - `KeyConfig/icon`
  - `KeyConfig/key`
  - `KeyConfig/backgrnd`
  - `KeyConfig/backgrnd2`
  - `KeyConfig/backgrnd3`
  - `KeyConfig/button:*` (various buttons)

### 3. UIMiniMap
- **File**: `IO/UITypes/UIMiniMap.cpp`
- **Primary Asset Path**: `UI/UIWindow2.img`
- **Sub-assets**:
  - `UIWindow2/MiniMap` (or `MiniMapSimpleMode` if simple mode is enabled)
  - `UIWindow2/MiniMap/ListNpc`
  - `Map/MapHelper.img` (for minimap markers)
- **Components**:
  - Map display (min/normal/max sizes)
  - Player position marker
  - NPC list
  - Portal markers
  - Various control buttons

### 4. UIChatBar
- **File**: `IO/UITypes/UIChatBar.cpp`
- **Primary Asset Path**: `UI/StatusBar3.img/chat/ingame`
- **Sub-assets**:
  - `chat/ingame/input`
  - `chat/ingame/view/max`
  - `chat/ingame/view/min`
  - `chat/ingame/view/drag`
- **Components**:
  - Chat input area
  - Chat display area (expandable)
  - Chat type buttons

### 5. UIBuffList
- **File**: `IO/UITypes/UIBuffList.cpp`
- **Assets**: 
  - Skill icons from `Skill/[skillid].img/skill/[skillid]/icon`
  - Item icons for consumable buffs
- **Components**:
  - Active buff icons with duration timers
  - Flashing effect when buffs are about to expire

### 6. UIStatusMessenger
- **File**: `IO/UITypes/UIStatusMessenger.cpp`
- **Assets**: None (text-only display)
- **Components**:
  - Floating status messages
  - Fade-out effect

## Potential v87 Compatibility Issues

1. **Asset Path Differences**:
   - The code expects `StatusBar3.img` which might be `StatusBar2.img` or `StatusBar.img` in v87
   - `UIWindow2.img` might be `UIWindow.img` in v87
   
2. **Missing Buttons/Features**:
   - The code has v87 compatibility checks for chat buttons (lines 96-100 in UIChatBar.cpp)
   - Some menu buttons might not exist in v87 (e.g., auction, monster collection)

3. **Resolution Support**:
   - The code handles multiple resolutions (800, 1024, 1280, 1366, 1920)
   - v87 might not support all these resolutions

4. **Gauge/Effect Differences**:
   - HP/MP gauge paths like `status/gauge/hp/layer:0` might differ
   - Experience bar effects might have different structures

## Recommendations

1. Check if v87 uses different asset file names:
   - `StatusBar.img` vs `StatusBar3.img`
   - `UIWindow.img` vs `UIWindow2.img`

2. Verify that all referenced sub-nodes exist in v87:
   - Menu button nodes
   - Gauge layer structures
   - Effect animations

3. Add fallback mechanisms for missing assets:
   - Default textures for missing buttons
   - Simplified UI modes for older versions