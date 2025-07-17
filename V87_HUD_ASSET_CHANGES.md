# V87 HUD Asset Path Changes Required

## Overview
The client code expects newer MapleStory asset paths that don't exist in v87. Here's a comprehensive list of all HUD UI elements that need path changes.

## 1. StatusBar (HP/MP/EXP/Buttons)
**File:** `IO/UITypes/UIStatusBar.cpp`

### Current Code Expects:
- `UI/StatusBar3.img/mainBar/*`
- `UI/StatusBar3.img/mainBar/EXPBar/*`
- `UI/StatusBar3.img/mainBar/menu/*`
- `UI/StatusBar3.img/mainBar/quickSlot/*`
- `UI/StatusBar3.img/mainBar/submenu/*`

### V87 Actually Has:
- `UI/StatusBar.img/*` (no "3", no "mainBar" subfolder)
- Simpler structure with basic buttons directly under StatusBar.img

### Required Changes:
```cpp
// Line 68 - Change from:
nl::node mainBar = nl::nx::UI["StatusBar3.img"]["mainBar"];
// To:
nl::node mainBar = nl::nx::UI["StatusBar.img"];
```

## 2. KeyConfig (Hotkeys/Quickslots)
**File:** `IO/UITypes/UIKeyConfig.cpp`

### Current Code Expects:
- `UI/StatusBar3.img/KeyConfig/*`

### V87 Actually Has:
- Keys are likely in `UI/StatusBar.img/` directly
- May have different button names

### Required Changes:
```cpp
// Line 42 - Change from:
nl::node KeyConfig = nl::nx::UI["StatusBar3.img"]["KeyConfig"];
// To:
nl::node KeyConfig = nl::nx::UI["StatusBar.img"];
```

## 3. MiniMap
**File:** `IO/UITypes/UIMiniMap.cpp`

### Current Code Expects:
- `UI/UIWindow2.img/MiniMap/*`
- `UI/UIWindow2.img/MiniMapSimpleMode/*`

### V87 Actually Has:
- Unknown - MiniMap might be in `UI/UIWindow.img/` or a separate file

### Required Changes:
```cpp
// Line 47 - Change from:
nl::node UIWindow2 = nl::nx::UI["UIWindow2.img"];
// To:
nl::node UIWindow = nl::nx::UI["UIWindow.img"];
// Then check if MiniMap exists under it
```

## 4. ChatBar
**File:** `IO/UITypes/UIChatBar.cpp`

### Current Code Expects:
- `UI/StatusBar3.img/chat/ingame/*`

### V87 Actually Has:
- Chat UI might be in `UI/StatusBar.img/` directly
- May have simpler structure without "ingame" subfolder

### Required Changes:
```cpp
// Line 34 - Change from:
nl::node ingame = nl::nx::UI["StatusBar3.img"]["chat"]["ingame"];
// To:
nl::node ingame = nl::nx::UI["StatusBar.img"]["chat"];
// Or possibly just:
nl::node ingame = nl::nx::UI["StatusBar.img"];
```

## 5. BuffList
**File:** `IO/UITypes/UIBuffList.cpp`

### Current Code Expects:
- `UI/StatusBar3.img/mainBar/buffIcon/*`

### V87 Actually Has:
- Buff icons might be in a different location

## 6. Item Inventory (Already Found)
**File:** `IO/UITypes/UIItemInventory.cpp`

### Current Code Expects:
- `UI/UIWindow2.img/Item/*` with complex structure

### V87 Actually Has:
- `UI/UIWindow.img/Equip/*` with only 10 items (basic buttons and backgrounds)

## Summary of Main Issues

1. **Version Suffix**: v87 uses older asset names without version numbers:
   - `StatusBar.img` instead of `StatusBar3.img`
   - `UIWindow.img` instead of `UIWindow2.img`

2. **Simpler Structure**: v87 has flatter folder structures:
   - No "mainBar" subfolder in StatusBar
   - No complex tab systems
   - Fewer buttons and options

3. **Missing Features**: Many UI elements don't exist in v87:
   - No tab-based inventory system
   - No complex button layouts
   - Simpler chat interface

## Recommended Approach

1. Add v87 compatibility checks:
```cpp
bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
nl::node statusbar = is_v87 ? nl::nx::UI["StatusBar.img"] : nl::nx::UI["StatusBar3.img"]["mainBar"];
```

2. Use fallback paths when assets are missing
3. Simplify UI layouts for v87 compatibility
4. Consider creating a V87AssetPaths utility class to centralize all path mappings

## Files That Need Updates
1. `UIStatusBar.cpp` - Status bar, HP/MP/EXP
2. `UIKeyConfig.cpp` - Hotkeys configuration
3. `UIMiniMap.cpp` - Minimap display
4. `UIChatBar.cpp` - Chat interface
5. `UIBuffList.cpp` - Buff icons
6. `UIItemInventory.cpp` - Inventory UI (already partially fixed)
7. Any other UI files that reference `StatusBar3.img` or `UIWindow2.img`