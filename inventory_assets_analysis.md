# Inventory UI Assets Analysis

Based on the asset paths you provided and the UIItemInventory.cpp implementation, here's an analysis of which assets are required:

## Critical Assets (Must Have)

### Main Container
- `UI/UIWindow2.img/Item` - Main container node

### Position Data (Required for slot layout)
- `UI/UIWindow2.img/Item/pos/slot_col` - Number of columns
- `UI/UIWindow2.img/Item/pos/slot_pos` - Starting position
- `UI/UIWindow2.img/Item/pos/slot_row` - Number of rows  
- `UI/UIWindow2.img/Item/pos/slot_space_x` - X spacing between slots
- `UI/UIWindow2.img/Item/pos/slot_space_y` - Y spacing between slots

### Background (At least one variant needed)
The code tries these in order:
1. `UI/UIWindow2.img/Item/productionBackgrnd`
2. `UI/UIWindow2.img/Item/backgrnd` (fallback)
3. `UI/UIWindow2.img/Item/Backgrnd` (fallback)

### Icons
- `UI/UIWindow2.img/Item/disabled` - Disabled slot overlay (used to calculate icon dimensions)

## Important Assets (UI won't look right without these)

### Tab Buttons
- `UI/UIWindow2.img/Item/Tab/enabled/0-5` - 6 enabled tab states
- `UI/UIWindow2.img/Item/Tab/disabled/0-5` - 6 disabled tab states

### Close Button
- `UI/Basic.img/BtClose3` - Standard close button

## Optional Assets (UI will work without these)

### Additional Backgrounds
- `UI/UIWindow2.img/Item/productionBackgrnd2`
- `UI/UIWindow2.img/Item/productionBackgrnd3`
- `UI/UIWindow2.img/Item/FullBackgrnd`
- `UI/UIWindow2.img/Item/FullBackgrnd2`
- `UI/UIWindow2.img/Item/FullBackgrnd3`

### New Item Indicators
- `UI/UIWindow2.img/Item/New/inventory`
- `UI/UIWindow2.img/Item/New/Tab0`
- `UI/UIWindow2.img/Item/New/Tab1`

### Other Icons
- `UI/UIWindow2.img/Item/activeIcon` - Projectile indicator

### Buttons (Normal Mode)
- `UI/UIWindow2.img/Item/AutoBuild/button:Coin`
- `UI/UIWindow2.img/Item/AutoBuild/button:Point`
- `UI/UIWindow2.img/Item/AutoBuild/button:Gather`
- `UI/UIWindow2.img/Item/AutoBuild/button:Sort`
- `UI/UIWindow2.img/Item/AutoBuild/button:Full`
- `UI/UIWindow2.img/Item/AutoBuild/button:Upgrade`
- `UI/UIWindow2.img/Item/AutoBuild/button:Appraise`
- `UI/UIWindow2.img/Item/AutoBuild/button:Extract`
- `UI/UIWindow2.img/Item/AutoBuild/button:Disassemble`
- `UI/UIWindow2.img/Item/AutoBuild/anibutton:Toad`

### Buttons (Full Mode)
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Small`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Coin`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Point`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Gather`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Sort`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Upgrade`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Appraise`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Extract`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Disassemble`
- `UI/UIWindow2.img/Item/FullAutoBuild/anibutton:Toad`
- `UI/UIWindow2.img/Item/FullAutoBuild/button:Cashshop`

## Asset Loading Behavior

Based on the code analysis:

1. **Background Loading**: The code has fallback logic for backgrounds. If `productionBackgrnd` is missing, it tries `backgrnd`, then `Backgrnd`.

2. **Default Values**: If position data is missing or invalid (0,0), the code uses default values:
   - Default position: (13, 117)
   - Default slots: 24
   - Default columns: 4
   - Default spacing: 36x35

3. **Error Handling**: Missing textures are logged but don't crash the UI. Empty textures are used as placeholders.

4. **Button States**: Extract and Disassemble buttons are disabled by default in the code.

## Potential Issues

If critical assets are missing:
1. **No container node**: UI won't initialize at all
2. **No position data**: Slots will use defaults but may look wrong
3. **No background**: UI will render but may have transparency issues
4. **No disabled icon**: Icon dimensions calculation fails, affecting slot rendering

To verify which assets exist in your NX files, you would need to either:
1. Run the CheckInventoryAssets.exe program
2. Use a NX file browser/editor to manually check
3. Add debug logging to UIItemInventory.cpp to log which assets load successfully