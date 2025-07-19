# MapleStory Map Editor v2

This branch adds a map editor mode to the MapleStory client without removing any existing functionality. The editor runs alongside the normal game client and can be activated with a command line flag.

## Usage

To start the client in map editor mode:
```
MapleStory.exe --editor
```

Without the flag, the client runs normally and connects to the server as usual.

## Features (Current)

- **Standalone Operation**: Editor mode bypasses login and server connection
- **Map Viewing**: Load and view any existing map
- **Basic Navigation**: Move around the map using existing controls
- **UI Integration**: Uses the game's existing UI system

## Architecture

The map editor is implemented as:
- A new UI state (`UIStateMapEditor`) that runs instead of the login state
- Command line parsing to detect `--editor` flag
- Configuration setting to track editor mode
- Direct map loading without server interaction

## Planned Features

1. **Map Format Parser**
   - Text-based format (YAML/JSON) with per-map palettes
   - Human and LLM-friendly design
   - Import/export functionality

2. **Editing Tools**
   - Tile placement palette
   - Object/NPC/Monster placement
   - Portal creation and editing
   - Property inspector

3. **File Operations**
   - New map creation
   - Save/Load map files
   - Map validation

4. **Playtesting**
   - Switch between edit and play modes
   - Test portals and spawns
   - Preview mob behavior

## Development Status

- [x] Command line flag parsing
- [x] Editor UI state
- [x] Skip server connection in editor mode
- [x] Load default map
- [x] Build and compile successfully
- [ ] Map format design
- [ ] Tile palette UI
- [ ] Object placement tools
- [ ] Save/Load functionality
- [ ] Playtest mode

## Map Format Design (Planned)

```yaml
# Example map format
palette:
  G : { id: 100450, name: "Grass_mid" }
  L : { id: 123010, name: "Ladder" }
  E : { id: 9410021, name: "Mob_Stirge" }
  
size: [20, 14]

grid: |
  GGGGGLGGGGG
  GGGGGLGGGGG
  GGGGGEGGGG
  
objects:
  player_start: [1, 1]
  portals:
    - { pos: [19, 5], dest: "map002" }
```

This format is designed to be:
- Easy for humans to read and edit
- Simple for LLMs to generate
- Compact yet expressive
- Version control friendly