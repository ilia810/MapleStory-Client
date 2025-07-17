# StatusBar v87 Research - Repomix for Researcher AI

## Problem Summary

We are working on HeavenClient, a MapleStory private server client, trying to make it compatible with original v87 GMS (Global MapleStory) data. The status bar (HP/MP/EXP bars) is not displaying correctly despite multiple attempts to fix the gauge textures.

**Current Issues:**
1. HP/MP/EXP bars still not showing correctly 
2. May be displaying "double bars" or "wrong bars"
3. Using v87 StatusBar.img structure which differs significantly from modern clients

## What We Need to Understand

The researcher needs to analyze the v87 StatusBar.img structure and determine:

1. **Asset Mapping**: Which specific textures from StatusBar.img should be used for HP, MP, and EXP gauges
2. **Structure Analysis**: How the v87 StatusBar.img hierarchy differs from modern StatusBar3.img
3. **Gauge Implementation**: Proper way to create Gauge objects using v87 assets
4. **Positioning Logic**: Where HP/MP/EXP bars should be positioned in v87 layout

## v87 StatusBar.img Complete Structure

Based on extraction from actual v87 data files:

```
StatusBar.img/
├── BtClaim/                    # Claim button assets
├── BtMenu/                     # Menu button assets  
├── QuickSlot/                  # Quick slot UI elements
├── QuickSlotD/                 # Quick slot (disabled?) elements
├── gauge/                      # *** CRITICAL: All gauge textures ***
│   ├── bar                     # Generic bar texture
│   ├── graduation              # Gauge scale/markings
│   ├── gray                    # Empty/gray state bar
│   ├── hpFlash/               # HP gauge animations
│   │   ├── 0                  # Frame 0 of HP animation
│   │   ├── 1                  # Frame 1 of HP animation  
│   │   ├── 2                  # Frame 2 of HP animation
│   │   ├── 3                  # Frame 3 of HP animation
│   │   └── 4                  # Frame 4 of HP animation
│   ├── mpFlash/               # MP gauge animations
│   │   ├── 0                  # Frame 0 of MP animation
│   │   ├── 1                  # Frame 1 of MP animation
│   │   ├── 2                  # Frame 2 of MP animation  
│   │   ├── 3                  # Frame 3 of MP animation
│   │   └── 4                  # Frame 4 of MP animation
│   └── tempExp                # Temporary/EXP gauge texture
├── chat                       # Chat UI element
├── chatTarget                 # Chat target element
└── quickSlot                  # Quick slot element
```

## Key Code Files

### 1. UIStatusBar.cpp - Main Implementation
```cpp
// Current v87 gauge creation logic (lines 288-309)
} else {
    // V87: Use proper HP/MP gauge textures
    nl::node statusBar = nl::nx::UI["StatusBar.img"];
    nl::node gauge = statusBar["gauge"];
    
    // Use dedicated HP and MP gauge textures
    if (!gauge["hpFlash"].name().empty() && !gauge["mpFlash"].name().empty()) {
        Texture hpTexture(gauge["hpFlash"]["0"]); // Use first frame of HP animation
        Texture mpTexture(gauge["mpFlash"]["0"]); // Use first frame of MP animation
        hpbar = Gauge(Gauge::Type::DEFAULT, hpTexture, hpmp_max, 0.0f);
        mpbar = Gauge(Gauge::Type::DEFAULT, mpTexture, hpmp_max, 0.0f);
    } else if (!gauge["bar"].name().empty()) {
        // Fallback to generic bar if flash textures not available
        Texture barTexture(gauge["bar"]);
        hpbar = Gauge(Gauge::Type::DEFAULT, barTexture, hpmp_max, 0.0f);
        mpbar = Gauge(Gauge::Type::DEFAULT, barTexture, hpmp_max, 0.0f);
    } else {
        // Create empty gauges if no textures found
        hpbar = Gauge();
        mpbar = Gauge();
    }
}
```

### 2. Gauge.h - Gauge Class Definition
```cpp
class Gauge {
public:
    enum Type : uint8_t {
        DEFAULT,
        CASHSHOP,
        WORLDSELECT
    };

    Gauge() {}
    Gauge(Type type, Texture front, int16_t maximum, float percent);
    Gauge(Type type, Texture front, Texture middle, int16_t maximum, float percent);
    Gauge(Type type, Texture front, Texture middle, Texture end, int16_t maximum, float percentage);

    void draw(const DrawArgument& args) const;
    void update(float target);
};
```

## Modern vs v87 Differences

### Modern Client Structure (StatusBar3.img):
```
StatusBar3.img/mainBar/
├── status/
│   ├── gauge/
│   │   ├── hp/layer:0          # HP gauge texture
│   │   ├── mp/layer:0          # MP gauge texture
│   │   └── number              # Number font
├── EXPBar/
│   ├── [resolution]/
│   │   ├── layer:gauge         # EXP gauge texture
│   │   └── layer:cover         # EXP cover layer
```

### v87 Structure (StatusBar.img):
```
StatusBar.img/
├── gauge/                      # All gauges directly here
│   ├── hpFlash/[0-4]          # HP animation frames
│   ├── mpFlash/[0-4]          # MP animation frames  
│   └── tempExp                # EXP gauge texture
```

## Research Questions

1. **Asset Verification**: Are `hpFlash`, `mpFlash`, and `tempExp` the correct textures for gauges?

2. **Missing Elements**: Are there background sprites, borders, or container elements we're missing from StatusBar.img?

3. **Positioning**: The current code sets v87 HP/MP position to `Point<int16_t>(50, 25)` - is this correct for v87 layout?

4. **Gauge Parameters**: Are we using correct width/height parameters for v87 gauges? Current: `hpmp_max = 139` (or 169 for width > 800)

5. **Background Integration**: Should gauges be drawn over background elements from StatusBar.img?

6. **Animation Handling**: Should we be using animation frames for HP/MP or just frame 0?

## Expected Behavior

In a working v87 client, the status bar should show:
- HP bar (red/health colored) 
- MP bar (blue/mana colored)
- EXP bar (yellow/experience colored)
- All positioned correctly within the status bar background
- Proper proportional filling based on current/max values

## Current Status

Multiple iterations of gauge texture assignment have been attempted:
1. Using generic `gauge["bar"]` for all - resulted in identical textures
2. Using `hpFlash["0"]` and `mpFlash["0"]` - still not displaying correctly
3. Various positioning adjustments - bars still misaligned

The issue may be:
- Wrong texture selection
- Missing background/container elements
- Incorrect positioning calculations
- Gauge constructor parameters
- Drawing order/layering issues

## Files to Analyze

1. `UIStatusBar.cpp` - Main status bar implementation
2. `UIStatusBar.h` - Status bar class definition  
3. `Gauge.cpp/Gauge.h` - Gauge rendering logic
4. `V87_HUD_ASSETS_ACTUAL.md` - Documented v87 asset structure
5. `StatusBar_v87_Investigation_Summary.txt` - Previous research

## Request for Researcher

Please analyze the provided code and v87 asset structure to determine:

1. The correct mapping of StatusBar.img assets to HP/MP/EXP display
2. Proper Gauge constructor usage for v87 
3. Any missing background or positioning elements
4. Recommended fixes for the current implementation

Focus on understanding the logical relationship between the flat v87 StatusBar.img structure and the hierarchical modern structure, and how to properly adapt the rendering code.