# V87 Inventory Asset Analysis - CRITICAL FINDINGS

## The Problem
The current `UIItemInventory.cpp` is looking for assets that **don't exist in v87**:
- Code expects: `UI/UIWindow2.img/Item/*`
- V87 has: `UI/UIWindow.img/Equip/*`

## What V87 Actually Has

### In `UI/UIWindow.img/Equip/`:
```
├─ BtCashshop     (Cash shop button)
├─ BtDetail       (Detail button)
├─ BtPet1-3       (Pet buttons)
├─ BtPetEquipHide/Show (Pet equipment toggle)
├─ backgrnd       (Main background)
├─ FullBackgrnd   (Full mode background)
├─ pet            (Pet UI element)
```

### What's MISSING in V87:
1. **No Tab System** - No Tab/enabled or Tab/disabled folders
2. **No Position Data** - No pos/slot_col, slot_row, etc.
3. **No Item Buttons** - No AutoBuild or FullAutoBuild buttons
4. **No New Indicators** - No New/inventory markers
5. **No Icons** - No activeIcon or disabled overlays

## Why The Inventory Freezes
The code tries to load non-existent assets and likely fails during initialization, causing the UI to freeze or not render properly.

## Solution Required
The `UIItemInventory.cpp` needs to be rewritten to:
1. Use `UIWindow.img/Equip` instead of `UIWindow2.img/Item`
2. Remove dependencies on missing assets
3. Use hardcoded positions instead of pos data
4. Simplify the UI to match v87's structure

## Alternative: Check Item Tab
V87 might have inventory UI in a different location. Let me check if there's an "Item" tab somewhere else...