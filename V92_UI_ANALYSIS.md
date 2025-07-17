# V92 UI.wz Analysis and Discrepancies

## Overview
This document analyzes the v92 UI.wz files and documents discrepancies with what the HeavenClient expects (based on v83 compatibility).

## Key Findings

### 1. Login Screen (Login.img.json)

#### What Client Expects (V83UIAssets.h):
- `Login.img/Common/frame` - Background image
- `Login.img/Common/BtStart` - Login button  
- `Login.img/Common/BtExit` - Quit button
- `Login.img/Title` - Title section
- `Login.img/TabD`, `Login.img/TabE` - Tab buttons

#### What V92 Actually Has:
- ✅ `Common/BtStart` - Exists at line 3400
- ✅ `Common/BtStart2` - Alternative start button at line 64  
- ✅ `Common/BtExit` - Exists at line 3509
- ✅ `Common/frame` - Exists at line 3635
- ✅ `Title` section - Exists at line 3960
- ✅ `Title/BtLogin` - Login button exists at line 4767
- ✅ `Title/BtQuit` - Quit button exists at line 5295
- ❌ `TabD`, `TabE` - Not found in Common section

#### Discrepancies:
1. **Login Button**: Client looks for `Common/BtStart` but v92 also has `Title/BtLogin`
2. **Quit Button**: Client looks for `Common/BtExit` but v92 also has `Title/BtQuit`
3. **Tab Buttons**: Client expects TabD/TabE but these might be in a different location in v92

### 2. World Select (Login.img.json)

#### What Client Expects:
- `Login.img/Common/selectWorld` - Background
- `Login.img/WorldSelect/BtWorld/[0-22]` - World buttons
- `Login.img/WorldSelect/BtChannel/[0-19]` - Channel buttons

#### What V92 Has:
- ✅ `WorldSelect` section - Exists at line 5473
- Need to check internal structure for BtWorld and BtChannel

### 3. Character Select (Login.img.json)

#### What Client Expects:
- `Login.img/CharSelect/BtNew` - New character button
- `Login.img/CharSelect/backgrnd` - Background

#### What V92 Has:
- ✅ `CharSelect` section - Exists at line 10556
- Need to verify BtNew and backgrnd existence

### 4. UI Window Elements (UIWindow.img.json)

Need to analyze UIWindow.img.json for:
- Inventory backgrounds
- Tab structures
- Window frames
- Close buttons

### 5. Status Bar (StatusBar.img.json)

Need to analyze StatusBar.img.json for:
- Base background
- HP/MP bars
- EXP bar
- Menu buttons

### 4. UI Window Elements (UIWindow.img.json)

#### What Client Expects:
- `UIWindow.img/Item/backgrnd` - Inventory background
- `UIWindow.img/Item/Tab/[enabled|disabled]/[0-4]` - Tab structures
- `Basic.img/BtClose` or `UIWindow.img/BtClose` - Close buttons

#### What V92 Has:
- ✅ `Item/backgrnd` - Exists (line 3-14)
- ✅ `Item/Tab/enabled/[0-4]` - Tab structure exists with proper numbering
- ✅ `Item/Tab/disabled/[0-4]` - Disabled tabs exist
- ✅ `BtUIClose` - Close button exists at root level
- ✅ `BtUIClose2` - Alternative close button

### 5. Status Bar (StatusBar.img.json)

#### What Client Expects:
- `StatusBar.img/base/backgrnd` - Base background

#### What V92 Has:
- ✅ `base/backgrnd` - Exists (lines 5-10)
- ✅ `base/backgrnd2` - Alternative background (lines 11-16)
- ✅ `gauge/bar` - HP/MP/EXP bars
- ✅ `base/quickSlot` - Quick slot UI

## Key Discrepancies Summary

1. **Button Locations**: V92 has buttons in both `Common` and `Title` sections
2. **Missing Channel Buttons**: `BtChannel` not found in WorldSelect
3. **Close Button Naming**: V92 uses `BtUIClose` instead of `BtClose`
4. **Multiple Backgrnd Options**: V92 often has `backgrnd` and `backgrnd2`
5. **Tab/TabE Missing**: Login tabs not in Common section

## Recommended Fixes

### 1. Update V83UIAssets.h to handle v92 paths:

```cpp
static nl::node getLoginButton(const std::string& buttonName) {
    nl::node login = nl::nx::UI["Login.img"];
    
    if (buttonName == "Login") {
        // v92 has login button in Title section
        buttonPaths = {
            "Title/BtLogin",       // v92 primary location
            "Common/BtStart",      // v83/v92 alternative
            "Common/BtStart2"      // v92 alternative
        };
    } else if (buttonName == "Quit") {
        // v92 has quit button in Title section
        buttonPaths = {
            "Title/BtQuit",        // v92 primary location
            "Common/BtExit"        // v83/v92 alternative
        };
    } else if (buttonName == "New") {
        // BtNew exists in both WorldSelect and CharSelect
        buttonPaths = {
            "CharSelect/BtNew",    // Primary for character creation
            "WorldSelect/BtNew",   // Alternative location
            "BtNew"                // Direct fallback
        };
    }
    // ... rest of logic
}

static nl::node getCloseButton() {
    // v92 uses different naming
    nl::node btn = nl::nx::UI["UIWindow.img"]["BtUIClose"];
    if (btn) return btn;
    
    btn = nl::nx::UI["UIWindow.img"]["BtUIClose2"];
    if (btn) return btn;
    
    // Original v83 paths
    btn = nl::nx::UI["Basic.img"]["BtClose"];
    if (btn) return btn;
    
    return nl::node();
}
```

### 2. Add v92 detection:

```cpp
static bool isV92Mode() {
    // Check for v92-specific elements
    nl::node titleBtLogin = nl::nx::UI["Login.img"]["Title"]["BtLogin"];
    nl::node btUIClose = nl::nx::UI["UIWindow.img"]["BtUIClose"];
    return titleBtLogin.data_type() != nl::node::type::none || 
           btUIClose.data_type() != nl::node::type::none;
}
```

### 3. Update missing assets handling:

```cpp
static nl::node getChannelButton(int channel) {
    // v92 might not have BtChannel at all
    nl::node worldSelect = nl::nx::UI["Login.img"]["WorldSelect"];
    
    // Try BtChannel first
    nl::node btChannel = worldSelect["BtChannel"];
    if (btChannel) {
        return btChannel[std::to_string(channel)];
    }
    
    // Fallback: use generic button from Common
    return nl::nx::UI["Login.img"]["Common"]["BtOk"];
}
```

## Next Steps

1. Update V83UIAssets.h with v92 support
2. Test each UI element in-game
3. Create fallback mechanisms for missing assets
4. Consider creating V92UIAssets.h as a separate compatibility layer