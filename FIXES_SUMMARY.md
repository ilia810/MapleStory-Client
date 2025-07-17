# MapleStory Client Fixes Summary

## ✅ Fixed Issues

### 1. Inventory UI Freeze (I key)
**Status**: FIXED
- Changed loop to start at slot 1 instead of 0
- Added guard to ignore slot 0 in update_slot()
- File: `UIItemInventory.cpp`

### 2. KeyConfig UI Freeze (K key)  
**Status**: FIXED
- Added check to skip item ID 0
- File: `UIKeyConfig.cpp`

### 3. Camera Stops Following on Small Maps
**Status**: FIXED
- Added centering logic for maps smaller than viewport
- File: `Camera.cpp`

### 4. Missing Portals
**Status**: FIXED
- Added v87 fallback check for missing "portal" and "life" nodes
- File: `Stage.cpp`

## ⚠️ Server-Side Issues (Not Client Problems)

### 1. Missing NPCs
**Explanation**: NPCs are spawned through network packets from the server, not loaded from client map files. If NPCs are missing, it means:
- The server is not sending SpawnNpc packets
- OR the server cannot find NPC data in its files
- This is a **server configuration issue**, not a client bug

### 2. Missing Mobs/Monsters  
**Explanation**: Same as NPCs - mobs are spawned by the server through SpawnMob packets

## Testing Checklist

✅ **Inventory (I)** - Opens without freezing  
✅ **Key Config (K)** - Opens without freezing  
✅ **Camera** - Follows character on all map sizes  
✅ **Portals** - Visible and functional  
⚠️ **NPCs** - Requires server fix  
⚠️ **Mobs** - Requires server fix  

## Compilation Status
All fixes compile successfully except:
- Remove the incorrect `npcs = MapNpcs(src["life"]);` line (already removed)

The client is now fully functional for all client-side features. NPC/Mob spawning requires server-side configuration fixes.