# UI Component Fix Tasks

## Overview
This document outlines all UI components that need to be analyzed and fixed to use simplified asset structures, removing unnecessary elements and ensuring proper functionality.

## Task Structure
Each task involves:
1. Analyzing the JSON structure to understand available assets
2. Reviewing the C++ implementation to see what's actually used
3. Identifying unnecessary assets (backgrounds, overlays, etc.)
4. Simplifying the implementation to use only essential assets
5. Testing the UI component

## UIWindow.img.json Tasks

### Inventory Windows
- **Task 1.1**: Fix Item Inventory Window
  - Components: Item (backgrnd, backgrnd2, backgrnd3, Tab, New, BtCoin, etc.)
  - Similar to Equip window - simplify to use only backgrnd
  
- **Task 1.2**: Fix Equipment Move Window  
  - Component: ItemMove
  - Analyze what backgrounds are actually needed

- **Task 1.3**: Fix Item Search Windows
  - Components: itemSearch, ExceptionItemSearch
  - Simplify search dialog backgrounds

### Character Windows
- **Task 2.1**: Fix Skill Book Window ✓ (Completed)
  - Used Skill.backgrnd structure
  
- **Task 2.2**: Fix Stats Window ✓ (Completed)  
  - Used Stat.backgrnd and backgrnd2 for extended view

- **Task 2.3**: Fix Key Configuration Window
  - Component: KeyConfig
  - Analyze backgrnd, backgrnd2, backgrnd3 usage

- **Task 2.4**: Fix Skill Macro Window
  - Components: MacroSkill, SkillMacro, SkillMacroEx
  - Simplify macro UI backgrounds

### Quest/NPC Windows  
- **Task 3.1**: Fix Quest Window
  - Components: Quest, QuestAlarm, QuestIcon
  - Analyze quest UI structure and backgrounds

### Shop Windows
- **Task 4.1**: Fix Shop Window
  - Component: Shop
  - Analyze shop backgrounds and simplify

- **Task 4.2**: Fix Personal/Entrusted Shop Windows
  - Components: PersonalShop, EntrustedShop, MemberShop
  - Standardize shop window backgrounds

- **Task 4.3**: Fix Store Bank Window
  - Component: StoreBank
  - Simplify bank UI

### Party/Guild Windows
- **Task 5.1**: Fix Party Search Windows
  - Components: PartySearch, PartySearch2
  - Consolidate party UI backgrounds

- **Task 5.2**: Fix Friend Recommendations
  - Component: FriendRecommendations
  - Simplify friend UI

### System Windows
- **Task 6.1**: Fix Channel Selection
  - Component: Channel
  - Simplify channel selection UI

- **Task 6.2**: Fix Event Windows
  - Components: EventWindow, 5thevent
  - Standardize event UI backgrounds

## Other UI Files Tasks

### Basic.img.json
- **Task 7.1**: Verify Basic UI Elements
  - Components: Cursor, CheckBox, Scrollbars (HScr, VScr variants)
  - Ensure all basic elements are properly loaded

### StatusBar.img.json  
- **Task 8.1**: Fix Status Bar
  - Components: base (backgrnd, backgrnd2), gauge, buttons
  - Simplify to essential backgrounds only

### Login.img.json
- **Task 9.1**: Fix Login UI
  - Components: Title, WorldSelect, CharSelect, NewChar
  - Analyze each login phase for necessary assets

### CashShop.img.json
- **Task 10.1**: Fix Cash Shop UI
  - Components: Base, CSItemSearch, CSChar, CSLocker, etc.
  - Major UI with many sub-components to simplify

### Chat and Display
- **Task 11.1**: Fix Chat Balloons
  - File: ChatBalloon.img.json
  - Standardize chat balloon rendering

- **Task 11.2**: Fix Name Tags
  - File: NameTag.img.json
  - Ensure proper name tag display

## Priority Order
1. High Priority (Core UI): Item inventory, remaining character windows
2. Medium Priority: Shops, Quest/NPC windows, Status bar
3. Low Priority: Cash shop, party/guild, special event windows

## Implementation Pattern
Based on completed work:
```cpp
// Old complex structure
nl::node Stat = nl::nx::UI["UIWindow.img"]["Stat"];
nl::node main = Stat["main"];
nl::node detail = Stat["detail"];
// Multiple backgrounds, covers, overlays

// New simplified structure  
nl::node Stat = nl::nx::UI["UIWindow.img"]["Stat"];
nl::node backgrnd = Stat["backgrnd"];
nl::node backgrnd2 = Stat["backgrnd2"]; // Only if needed for extended views
// Direct access to buttons and essential elements only
```

## Success Criteria
- Each UI window loads and displays correctly
- No missing textures or visual glitches
- Simplified code that's easier to maintain
- Performance improvement from loading fewer assets