# V87 HUD Fixes Summary

## The Problem
The HUD wasn't showing because the code was looking for assets in the wrong locations. V87 has a completely different asset structure than modern MapleStory versions.

## Key Differences Found

### StatusBar Assets (UIStatusBar.cpp)
**Modern Structure:**
- `UI/StatusBar3.img/mainBar/status/*`
- `UI/StatusBar3.img/mainBar/EXPBar/*`
- `UI/StatusBar3.img/mainBar/menu/*`

**V87 Structure:**
- `UI/StatusBar.img/base/*` - Basic backgrounds and UI elements
- `UI/StatusBar.img/gauge/*` - HP/MP gauges (hpFlash, mpFlash)
- `UI/StatusBar.img/BtMenu` - Menu button directly
- No mainBar subfolder, no separate EXPBar folder

### Chat Assets (UIChatBar.cpp)
**Modern Structure:**
- `UI/StatusBar3.img/chat/ingame/input/*`
- `UI/StatusBar3.img/chat/ingame/view/*`

**V87 Structure:**
- `UI/StatusBar.img/base/chat` - Just a single chat image
- No complex chat UI structure
- Chat functionality is very basic

### MiniMap Assets (UIMiniMap.cpp)
**Modern Structure:**
- `UI/UIWindow2.img/MiniMap/*`

**V87 Structure:**
- MiniMap UI elements might not exist in v87
- Map markers are in `Map/MapHelper.img/minimap/*`

## Fixes Applied

### 1. UIStatusBar.cpp
- Added v87 detection
- Maps gauge assets to `StatusBar.img/gauge/hpFlash` and `mpFlash`
- Uses `base` folder for backgrounds
- Handles missing charset/number assets

### 2. UIKeyConfig.cpp
- Added v87 detection
- Looks for KeyConfig assets directly under `StatusBar.img`

### 3. UIMiniMap.cpp
- Added v87 detection
- Handles missing MiniMap nodes gracefully
- Only creates buttons if nodes exist

### 4. UIChatBar.cpp
- Added v87 detection
- Creates placeholder textures for missing assets
- Uses default dimensions when textures don't exist
- Skips button creation for v87

## Why It Wasn't Working
1. The code expected nested folder structures that don't exist in v87
2. Many UI features simply don't exist in v87 (complex chat, tabs, etc.)
3. Asset names are different (StatusBar.img vs StatusBar3.img)

## Testing Needed
The HUD should now display basic elements:
- HP/MP bars (using hpFlash/mpFlash animations)
- Basic status display
- Simplified chat (if any)
- Basic minimap (if supported)

Some features will be missing or simplified because v87 doesn't have the assets for them.