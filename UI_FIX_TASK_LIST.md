# UI Component Fix Task List

## Completed Tasks ✓
- [x] **Equip Window** - Simplified to use only UI.UIWindow.backgrnd
- [x] **Stats Window** - Uses Stat.backgrnd and backgrnd2 for extended view
- [x] **Skill Book Window** - Uses Skill.backgrnd structure

## High Priority Tasks (Core UI)

### 1. Item Inventory Window
**File:** UIItemInventory.cpp  
**JSON:** UIWindow.img.json → Item
**Current Issues:** Multiple backgrounds (backgrnd, backgrnd2, backgrnd3), unnecessary overlays
**Fix:** Simplify to use only Item.backgrnd
**Components to check:**
- Tab structure
- New item indicator
- Coin display

### 2. Key Configuration Window  
**File:** UIKeyConfig.cpp
**JSON:** UIWindow.img.json → KeyConfig
**Fix:** Analyze and simplify backgrounds

### 3. Quest Log Window
**File:** UIQuestLog.cpp  
**JSON:** UIWindow.img.json → Quest
**Fix:** Simplify quest window backgrounds

### 4. NPC Talk Window
**File:** UINpcTalk.cpp
**JSON:** UIWindow.img.json → (check for NPC-related components)
**Fix:** Simplify dialog backgrounds

## Medium Priority Tasks

### 5. Shop Window
**File:** UIShop.cpp
**JSON:** UIWindow.img.json → Shop  
**Fix:** Simplify shop UI backgrounds

### 6. Status Bar
**File:** UIStatusBar.cpp
**JSON:** StatusBar.img.json
**Components:** base.backgrnd, base.backgrnd2
**Fix:** Use only essential backgrounds

### 7. Chat System
**Files:** UIChat*.cpp
**JSON:** ChatBalloon.img.json
**Fix:** Standardize chat balloon rendering

## Low Priority Tasks

### 8. Cash Shop
**File:** UICashShop.cpp (if exists)
**JSON:** CashShop.img.json
**Fix:** Major UI with many sub-components to simplify

### 9. Login UI
**File:** UILogin*.cpp
**JSON:** Login.img.json
**Fix:** Analyze each login phase for necessary assets

### 10. Party/Guild Windows
**Files:** UIParty*.cpp, UIGuild*.cpp
**JSON:** UIWindow.img.json → PartySearch, GuildBBS.img.json
**Fix:** Consolidate backgrounds

## Implementation Pattern

```cpp
// Before (complex):
nl::node Window = nl::nx::UI["UIWindow.img"]["WindowName"];
nl::node main = Window["main"];
nl::node detail = Window["detail"];
sprites.emplace_back(main["backgrnd"]);
sprites.emplace_back(main["backgrnd2"]);
sprites.emplace_back(main["backgrnd3"]);

// After (simplified):
nl::node Window = nl::nx::UI["UIWindow.img"]["WindowName"];
nl::node backgrnd = Window["backgrnd"];
sprites.emplace_back(backgrnd);
```

## Testing Checklist for Each Fix
- [ ] Window opens correctly
- [ ] All UI elements visible
- [ ] No missing textures
- [ ] Buttons functional
- [ ] Text properly aligned
- [ ] Window draggable
- [ ] Close button works

## Notes
- Always check the JSON structure first to understand available assets
- Remove loading of non-essential backgrounds (covers, overlays, etc.)
- Keep extended/detail backgrounds only if they serve a functional purpose
- Test each change thoroughly before moving to the next window