# V87 HUD Assets - Actual Paths from Code Analysis

This document lists the ACTUAL asset paths used by v87 MapleStory for HUD elements, based on code analysis of the HeavenClient source.

## 1. Status Bar (HP/MP/EXP)

**Source:** UIStatusBar.cpp
- Root: `UI.wz/StatusBar.img` (v87) or `UI.wz/StatusBar3.img` (later versions)
- For v87: Assets are directly under `StatusBar.img`

### Key Assets:
```
UI.wz/StatusBar.img/
├── status/              # Main status bar container
│   ├── backgrnd         # Background texture
│   ├── layer:cover      # Cover layer
│   ├── layer:Lv         # Level indicator
│   ├── gauge/           # HP/MP gauges
│   │   ├── hp/
│   │   │   └── layer:0  # HP bar texture
│   │   ├── mp/
│   │   │   └── layer:0  # MP bar texture
│   │   └── number       # Number font for HP/MP
│   └── lvNumber         # Level number font
├── EXPBar/              # Experience bar
│   ├── backgrnd         # EXP bar background
│   ├── [resolution]/    # Resolution-specific assets (800, 1024, 1280, etc.)
│   │   ├── layer:back   # Background layer
│   │   ├── layer:gauge  # EXP gauge fill
│   │   └── layer:cover  # Cover layer
│   ├── layer:effect     # EXP gain effect
│   └── number           # Number font for stats
├── quickSlot/           # Quick slot UI
│   ├── backgrnd         # Quick slot background
│   ├── layer:cover      # Quick slot cover
│   ├── button:Fold      # Fold button
│   ├── button:Fold800   # 800x600 fold button
│   ├── button:Extend    # Extend button
│   └── button:Extend800 # 800x600 extend button
├── menu/                # Menu buttons
│   ├── button:CashShop
│   ├── button:Menu
│   ├── button:Setting
│   ├── button:Character
│   ├── button:Community
│   └── button:Event
└── submenu/             # Submenu elements
    └── backgrnd/
        ├── 0
        ├── 1
        └── 2
```

### Actual Status Bar Assets Found in UI_current.txt:
```
StatusBar.img (18 items)
├── BtClaim/
├── BtMenu/
├── QuickSlot/
├── QuickSlotD/
├── gauge/ (6 items)
│   ├── bar
│   ├── graduation
│   ├── gray
│   ├── hpFlash/ (5 items with animation frames)
│   ├── mpFlash/ (5 items with animation frames)
│   └── tempExp
├── chat
├── chatTarget
└── quickSlot
```

## 2. Minimap

**Source:** UIMiniMap.cpp
- For v87: Check `UI.wz/UIWindow.img` first (not UIWindow2.img)
- Minimap node might not exist in v87, code has fallback

### Key Assets:
```
UI.wz/UIWindow.img/
├── MiniMap/             # Normal mode minimap
│   ├── BtMin            # Minimize button
│   ├── BtMax            # Maximize button
│   ├── BtSmall          # Small map button
│   ├── BtBig            # Big map button
│   ├── BtMap            # World map button
│   ├── BtNpc            # NPC list button
│   ├── Min/             # Minimized view
│   │   ├── Center
│   │   ├── Left
│   │   └── Right
│   ├── MinMap/          # Normal view
│   │   ├── c            # Center
│   │   ├── e            # East
│   │   ├── w            # West
│   │   ├── n            # North
│   │   ├── s            # South
│   │   ├── nw           # Northwest
│   │   ├── ne           # Northeast
│   │   ├── sw           # Southwest
│   │   └── se           # Southeast
│   ├── MaxMap/          # Maximized view
│   │   └── [same structure as MinMap]
│   ├── iconNpc          # NPC icon for selection
│   └── ListNpc/         # NPC list window
│       └── [window frame parts]
└── MiniMapSimpleMode/   # Simple mode variant
    ├── DefaultHelper    # Default helper marker
    └── Window/
        ├── Min/
        ├── Normal/
        └── Max/
```

### Map Helper Assets:
```
Map.wz/MapHelper.img/
├── minimap/             # Minimap markers
│   ├── user             # Player marker
│   ├── portal           # Portal marker
│   ├── npc              # NPC marker
│   └── another          # Other player marker
└── mark/                # Map type markers
    └── [mapMark types]  # Various map mark icons
```

## 3. Chat Bar

**Source:** UIChatBar.cpp
- For v87: Check `UI.wz/StatusBar.img/chat`
- Note: v87 might not have the "ingame" subfolder

### Key Assets:
```
UI.wz/StatusBar.img/
└── chat/                # Chat UI root
    ├── ingame/          # In-game chat (might not exist in v87)
    │   ├── input/       # Input area
    │   │   ├── layer:backgrnd    # Background
    │   │   └── layer:chatEnter   # Chat enter button
    │   └── view/        # Chat view area
    │       ├── max/     # Maximized view
    │       │   ├── top
    │       │   ├── center
    │       │   └── bottom
    │       ├── min/     # Minimized view
    │       │   ├── top
    │       │   ├── center
    │       │   └── bottom
    │       ├── drag     # Drag handle
    │       └── btMin/   # Minimize button
    │           └── normal/0
    └── [direct assets]  # In v87, assets might be directly here
```

## 4. Key Differences for v87

1. **No UIWindow2.img** - v87 uses `UIWindow.img`
2. **No StatusBar3.img** - v87 uses `StatusBar.img`
3. **Direct asset placement** - Some assets in v87 are directly under the main nodes without intermediate folders
4. **Fallback handling** - Code checks for empty nodes and provides fallbacks

## 5. Code Compatibility Patterns

The code uses this pattern to handle v87 compatibility:
```cpp
// Check if newer version exists
bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();

// Select appropriate root
nl::node statusBarRoot = is_v87 ? nl::nx::UI["StatusBar.img"] : nl::nx::UI["StatusBar3.img"];

// Handle different structures
nl::node mainBar = is_v87 ? statusBarRoot : statusBarRoot["mainBar"];
```

## 6. Resolution-Specific Assets

The status bar and EXP bar have resolution-specific variants:
- 800x600: Uses "status800" and specific button variants
- 1024x768, 1280x720, 1366x768, 1920x1080: Different positioning and sizing

## Important Notes

1. The actual asset structure in v87 is flatter than later versions
2. Many intermediate folders (like "mainBar", "ingame") don't exist in v87
3. The code has extensive fallback handling for missing nodes
4. Asset paths should be verified against the actual nx_structures files before use