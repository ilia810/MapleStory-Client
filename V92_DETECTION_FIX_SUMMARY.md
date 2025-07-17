# V92 Asset Detection Fix Summary

## Problem
The client was failing to detect v92 UI assets, causing it to search for buttons and UI elements in the wrong locations. This resulted in:
- Missing login buttons (HomePage, PasswdLost, EmailLost, EmailSave)
- Sprite creation failures
- Incorrect asset paths being used

## Root Cause
The `isV92Mode()` function in V83UIAssets.h was checking for `nl::node::type::none` incorrectly. In v92, the Title section exists as a container node with children, but its `data_type()` returns `none` because it's not a bitmap itself.

## Solution
Updated the `isV92Mode()` detection logic to:
1. Check if the Title section exists
2. Check if it has children (size() > 0)
3. As a fallback, check for specific v92 buttons

## Code Changes

### V83UIAssets.h
```cpp
static bool isV92Mode()
{
    nl::node login = nl::nx::UI["Login.img"];
    if (!login) {
        LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Login.img not found");
        return false;
    }
    
    nl::node title = login["Title"];
    if (!title) {
        LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Title section not found - not v92");
        return false;
    }
    
    // Check if Title section has any children (buttons)
    if (title.size() > 0) {
        LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Title section has " << title.size() << " children - v92 detected");
        return true;
    }
    
    // Alternative check: look for specific v92 buttons in Title
    if (title["BtLogin"] || title["BtQuit"] || title["BtHomePage"] || 
        title["BtPasswdLost"] || title["BtEmailLost"] || title["BtEmailSave"]) {
        LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Found v92 buttons in Title section");
        return true;
    }
    
    LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: No v92 indicators found");
    return false;
}
```

### UIWorldSelect.cpp
Added v92-specific handling for world selection textures:
```cpp
if (V83UIAssets::isV92Mode()) {
    // v92 structure: scroll animations for selected world
    nl::node scrollNode = nl::nx::UI["Login.img"]["WorldSelect"]["scroll"];
    if (scrollNode && scrollNode[world]) {
        nl::node worldScroll = scrollNode[world];
        if (worldScroll["0"]) {
            // Use the first frame of the scroll animation
            world_textures.emplace_back(worldScroll["0"]);
            LOG(LOG_DEBUG, "[UIWorldSelect] Found v92 world texture at scroll/" << world << "/0");
        }
    }
}
```

## Result
- V92 detection now works correctly
- Login buttons are found in the correct Title section locations
- World selection textures load from the correct scroll paths
- Auto-login functionality continues to work correctly

## Testing
Confirmed working by:
1. Running the client and checking debug logs
2. Verifying "isV92Mode: Title section has 16 children - v92 detected" appears
3. Confirming buttons are found at correct v92 paths (Title/BtLogin, etc.)
4. Auto-login successfully logs in and loads character