# NX Metadata Extraction Guide

## Overview

The current `_current.txt` files in this directory are **summaries** that lack the complete metadata needed for a proper AssetRegistry. To get ALL the data, we need to extract directly from the NX files.

## What We Need

For a complete AssetRegistry, we need:

1. **Full Asset Paths** - Complete paths to every image, sound, and data node
2. **Image Properties** - Width, height, origin points, delays for animations
3. **Animation Data** - Frame counts, delays, sequences
4. **UI Element States** - All button states (normal, pressed, disabled, etc.)
5. **Z-indices and Layering** - For proper rendering order
6. **Asset References** - Links between assets

## Available Tools

### 1. Enhanced C++ Extractor (`extract_all_nx_structures.cpp`)

**Features:**
- Extracts complete NX structure with metadata
- Captures image dimensions and properties
- Identifies animations and UI elements
- Generates full asset paths
- Creates categorized asset registry

**Usage:**
```bash
# Build
cl.exe /I"includes\NoLifeNx" /D USE_NX extract_all_nx_structures.cpp /link NoLifeNx.lib

# Run
nx_extractor.exe
```

**Output:**
- `nx_structures/*_structure.txt` - Full hierarchical structure with metadata
- `nx_structures/ASSET_REGISTRY_COMPLETE.txt` - Categorized asset lists
- `nx_structures/nx_summary.txt` - Overview and key locations

### 2. Metadata Extractor (`nx_metadata_extractor.cpp`)

**Features:**
- Outputs JSON format with complete metadata
- Extracts all properties (origin, delay, z-index, etc.)
- Creates AssetRegistry-compatible format
- Handles all node types

**Output:**
- `*_metadata.json` - Complete metadata in JSON
- `asset_patterns.json` - UI patterns and animations
- `asset_paths.txt` - Sample complete paths

### 3. Python Metadata Processor (`extract_full_metadata.py`)

**Features:**
- Processes existing dumps to extract more metadata
- Generates AssetRegistry format
- Pattern analysis for UI elements and animations

**Usage:**
```bash
python extract_full_metadata.py UI_current.txt
```

**Output:**
- `*_metadata.json` - Extracted metadata
- `*_registry.json` - AssetRegistry format

## Key Asset Locations

Based on the v83 client structure:

### UI Assets
- **Login Buttons**: `UI/Login.img/Title/Bt*`
- **Login Background**: `UI/Login.img/Notice/Loading/backgrnd`
- **Character Select**: `UI/Login.img/CharSelect/*`
- **World Select**: `UI/Login.img/WorldSelect/*`
- **Basic Buttons**: `UI/Basic.img/Bt*`

### Character Assets
- **Body Animations**: `Character/00002000.img/*`
- **Face**: `Character/Face/*`
- **Hair**: `Character/Hair/*`

### Map Assets
- **Backgrounds**: `Map/Back/*`
- **Tiles**: `Map/Tile/*`
- **Objects**: `Map/Obj/*`

## Next Steps

1. **Build the C++ Extractor**
   ```bash
   # Make sure NoLifeNx is built first
   cd NoLifeWzToNx
   cmake . && make
   
   # Then build the extractor
   build_nx_extractor.bat
   ```

2. **Run Full Extraction**
   - This will generate complete metadata files
   - Check `nx_structures/` for results

3. **Process with Python**
   - Use `extract_full_metadata.py` for additional processing
   - Generate AssetRegistry-compatible JSON

4. **Integrate with AssetRegistry**
   - Use the complete paths from extraction
   - Import metadata into your AssetRegistry system

## Important Notes

- The current `_current.txt` files show `(N items)` but don't include the actual items
- You need the full extraction to see what's inside those items
- Image assets need their origin points for proper positioning
- Animation frames need delays for correct playback
- UI elements need all states for proper interaction

## Example Output

From the enhanced extractor:
```
├─ BtOK 📁 (5 items) [UI_ELEMENT: normal,mouseOver,pressed,disabled,keyFocused]
  ├─ normal 🖼️ (42x16) {origin:(0,0)} PATH: UI/Basic.img/BtOK/normal
  ├─ mouseOver 🖼️ (42x16) {origin:(0,0)} PATH: UI/Basic.img/BtOK/mouseOver
  ├─ pressed 🖼️ (42x16) {origin:(0,0)} PATH: UI/Basic.img/BtOK/pressed
  ├─ disabled 🖼️ (42x16) {origin:(0,0)} PATH: UI/Basic.img/BtOK/disabled
  ├─ keyFocused 🖼️ (42x16) {origin:(0,0)} PATH: UI/Basic.img/BtOK/keyFocused
```

This gives you everything needed for the AssetRegistry!