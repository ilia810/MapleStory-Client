# V87 HUD Asset Changes Summary

## Completed Changes

I've successfully updated all HUD UI elements to be compatible with v87's asset structure. Here are the changes made:

### 1. UIStatusBar.cpp
**Changes:** Lines 68-79
- Added v87 compatibility check
- Uses `StatusBar.img` instead of `StatusBar3.img` for v87
- Removes "mainBar" subfolder navigation for v87

```cpp
bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
nl::node statusBarRoot = is_v87 ? nl::nx::UI["StatusBar.img"] : nl::nx::UI["StatusBar3.img"];
nl::node mainBar = is_v87 ? statusBarRoot : statusBarRoot["mainBar"];
```

### 2. UIKeyConfig.cpp
**Changes:** Lines 42-45
- Added v87 compatibility check
- Uses `StatusBar.img` directly instead of `StatusBar3.img/KeyConfig` for v87

```cpp
bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
nl::node statusBarRoot = is_v87 ? nl::nx::UI["StatusBar.img"] : nl::nx::UI["StatusBar3.img"];
nl::node KeyConfig = is_v87 ? statusBarRoot : statusBarRoot["KeyConfig"];
```

### 3. UIMiniMap.cpp
**Changes:** Lines 47-74, 80
- Added v87 compatibility check
- Uses `UIWindow.img` instead of `UIWindow2.img` for v87
- Added fallback for missing MiniMap nodes in v87
- Only creates buttons if nodes exist

```cpp
bool is_v87 = nl::nx::UI["UIWindow2.img"].name().empty();
nl::node UIWindow = is_v87 ? nl::nx::UI["UIWindow.img"] : nl::nx::UI["UIWindow2.img"];
```

### 4. UIChatBar.cpp
**Changes:** Lines 34-48
- Added v87 compatibility check
- Uses `StatusBar.img` instead of `StatusBar3.img` for v87
- Falls back to direct chat node if "ingame" subfolder doesn't exist

```cpp
bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
nl::node statusBarRoot = is_v87 ? nl::nx::UI["StatusBar.img"] : nl::nx::UI["StatusBar3.img"];
```

### 5. UIBuffList.cpp
**Status:** No changes needed
- This file doesn't reference StatusBar3.img or UIWindow2.img
- Uses skill icons from Skill NX files which are consistent across versions

### 6. UIItemInventory.cpp
**Status:** Already updated in previous session
- Uses `UIWindow.img/Equip` instead of `UIWindow2.img/Item` for v87

## Key Pattern Used

For all changes, I implemented a consistent pattern:
1. Check if the newer asset file exists: `bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();`
2. Select appropriate root node based on version
3. Handle structural differences (like missing subfolders in v87)
4. Add fallbacks for missing nodes

## Files Still Needing Updates

While I focused on HUD elements, these non-HUD UI files also reference the old asset paths and may need updates:
- UIWorldMap.cpp
- UIUserList.cpp
- UISkillBook.cpp
- UIShop.cpp
- UIQuestLog.cpp
- UIOptionMenu.cpp
- UINpcTalk.cpp
- UILoginNotice.cpp
- UIEvent.cpp
- UIChat.cpp
- UICharInfo.cpp
- UIChannel.cpp

These are lower priority as they're not part of the always-visible HUD.