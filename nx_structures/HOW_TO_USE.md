# How to Use NX Metadata Extraction

## Quick Start (Easiest Way)

1. **Run the extraction:**
   ```
   cd C:\HeavenClient\MapleStory-Client
   RUN_NX_EXTRACTION.bat
   ```

2. **Check the output in `nx_structures\` folder**

That's it! You'll get complete metadata files.

---

## What You Get

### 1. ASSET_REGISTRY_COMPLETE.txt
Complete asset paths with metadata:
```
UI/Basic.img/BtOK/normal [42x16]
UI/Basic.img/BtOK/mouseOver [42x16]
UI/Basic.img/BtOK/pressed [42x16]
```

### 2. UI_structure.txt (and other *_structure.txt)
Full hierarchy with ALL nodes:
```
├─ Basic.img 📁 (75 items)
  ├─ BtOK 📁 (5 items) [UI_ELEMENT: normal,mouseOver,pressed,disabled]
    ├─ normal 🖼️ (42x16) PATH: UI/Basic.img/BtOK/normal
    ├─ mouseOver 🖼️ (42x16) PATH: UI/Basic.img/BtOK/mouseOver
    ├─ pressed 🖼️ (42x16) PATH: UI/Basic.img/BtOK/pressed
    ├─ disabled 🖼️ (42x16) PATH: UI/Basic.img/BtOK/disabled
```

---

## Using the Data

### For AssetRegistry Code

Use the paths directly in your code:
```cpp
// From ASSET_REGISTRY_COMPLETE.txt
nl::node okButton = nl::nx::ui["Basic.img"]["BtOK"]["normal"];

// Or use the helper
Texture okBtn = get_texture("UI/Basic.img/BtOK/normal");
```

### Finding Assets

1. Open `ASSET_REGISTRY_COMPLETE.txt`
2. Search for what you need (e.g., "Login", "Button", "Character")
3. Copy the full path
4. Use it in your code

### Example Searches

**Find all login buttons:**
```
Search for: UI/Login.img/Title/Bt
```

**Find character animations:**
```
Search for: Character/00002000.img/
```

**Find UI backgrounds:**
```
Search for: backgrnd
```

---

## Manual Build (if batch file doesn't work)

### Option 1: With Visual Studio 2015
```bash
# Open VS2015 x64 Command Prompt
cd C:\HeavenClient\MapleStory-Client
cl.exe /I"includes\NoLifeNx" /D USE_NX extract_all_nx_structures.cpp /link /LIBPATH:"includes\NoLifeNx\nlnx\x64\Release" NoLifeNx.lib
nx_extractor.exe
```

### Option 2: With MinGW/g++
```bash
g++ -std=c++17 -DUSE_NX -I"includes/NoLifeNx" extract_all_nx_structures.cpp -o nx_extractor.exe -L"includes/NoLifeNx/nlnx/x64/Release" -lNoLifeNx
./nx_extractor.exe
```

---

## Python Alternative (for existing dumps)

If you can't build the C++ extractor:

```bash
cd nx_structures
python extract_full_metadata.py UI_current.txt

# This creates:
# - UI_metadata.json (structured data)
# - UI_registry.json (AssetRegistry format)
```

---

## Troubleshooting

**"NX files not found"**
- Make sure your .nx files are in the `Data\` folder
- They should be named: Base.nx, Character.nx, UI.nx, etc.

**"cl.exe not found"**
- Install Visual Studio 2015 Community
- Or use the Python script on existing dumps

**"NoLifeNx.lib not found"**
- The library is already included at: `includes\NoLifeNx\nlnx\x64\Release\NoLifeNx.lib`
- If missing, build NoLifeWzToNx first

---

## Next Steps

1. Run the extraction
2. Open `ASSET_REGISTRY_COMPLETE.txt` 
3. Find the assets you need
4. Use the paths in your AssetRegistry code

Example for your AssetRegistry:
```cpp
// Instead of guessing paths:
registry.add("UI/Login.img/Title/BtLogin/normal");
registry.add("UI/Login.img/Title/BtLogin/mouseOver");
registry.add("UI/Login.img/Title/BtLogin/pressed");

// You'll have the exact paths with dimensions!
```