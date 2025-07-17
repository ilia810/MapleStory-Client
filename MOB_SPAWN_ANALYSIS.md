# MapleStory Client - Mob Spawning Analysis

## Issue Summary
**Problem**: No mobs are spawning in maps when running the client
**Root Cause**: Mobs are spawned through server packets, not client-side code

## How Mob Spawning Works

### 1. Server-Driven Spawning
Mobs in MapleStory are spawned by the **server**, not the client. The client is a passive receiver that displays mobs when told to by the server.

### 2. Packet Flow
```
Server -> SPAWN_MOB (236) or SPAWN_MOB_C (238) packet -> Client
Client -> SpawnMobHandler -> MapMobs::spawn() -> MobSpawn queue
Game Loop -> MapMobs::update() -> Actual mob creation and rendering
```

### 3. Code Flow Analysis

#### Server Packets
- **SPAWN_MOB (236)**: Spawns a regular mob
- **SPAWN_MOB_C (238)**: Spawns a mob with controller assignment

#### Packet Handlers
Located in `Net/Handlers/MapObjectHandlers.cpp`:

```cpp
void SpawnMobHandler::handle(InPacket& recv) const {
    int32_t oid = recv.read_int();
    recv.read_byte(); // 5 if controller == null
    int32_t id = recv.read_int();
    // ... packet parsing ...
    
    Stage::get().get_mobs().spawn(
        { oid, id, 0, stance, fh, effect == -2, team, position }
    );
}
```

#### Client-Side Processing
Located in `Gameplay/MapleMap/MapMobs.cpp`:

```cpp
void MapMobs::spawn(MobSpawn&& spawn) {
    spawns.emplace(std::move(spawn));  // Adds to queue
}

void MapMobs::update(const Physics& physics) {
    for (; !spawns.empty(); spawns.pop()) {
        const MobSpawn& spawn = spawns.front();
        // ... process spawn from queue ...
        mobs.add(spawn.instantiate());  // Actually creates the mob
    }
}
```

## Why Mobs Don't Spawn

### 1. No Server Connection
If the client is not connected to a MapleStory server, no spawn packets will be received.

**Check**: `Session::get().is_connected()`

### 2. Server Not Configured for Spawning
The server may not be configured to spawn mobs on the current map.

### 3. Game Loop Not Running
The `MapMobs::update()` method must be called regularly to process the spawn queue.

### 4. Map Not Loaded
Mobs can only spawn in a loaded map.

## Testing Mob Spawning

### Manual Testing (Requires Server)
1. Connect to a MapleStory server
2. Load a map that should have mobs
3. Wait for server to send spawn packets
4. Observe if mobs appear

### Programmatic Testing
```cpp
// Check if connected to server
if (!Session::get().is_connected()) {
    std::cout << "No server connection - mobs cannot spawn!" << std::endl;
    return;
}

// Manually create a mob spawn (for testing only)
MobSpawn spawn(oid, mobId, mode, stance, fh, newspawn, team, position);
Stage::get().get_mobs().spawn(std::move(spawn));

// Update mobs (normally done in game loop)
// stage.get_mobs().update(physics);
```

## Solutions

### 1. For Development/Testing
Create a **mock server** or **offline mode** that sends spawn packets:

```cpp
void sendMockSpawnPackets() {
    // Create mock spawn packets for testing
    OutPacket spawn_packet(236); // SPAWN_MOB
    spawn_packet.write_int(12345); // oid
    spawn_packet.write_byte(5);    // controller flag
    spawn_packet.write_int(100100); // mob id (Orange Mushroom)
    // ... write other packet data ...
    
    // Process packet through handler
    SpawnMobHandler handler;
    InPacket recv(spawn_packet.getBytes());
    handler.handle(recv);
}
```

### 2. For Production
- Ensure server is running and configured correctly
- Verify server sends spawn packets for the current map
- Check that client is properly connected to server

### 3. For Debugging
Enable packet logging to see if spawn packets are being received:

```cpp
// In PacketSwitch.cpp, add logging
case SPAWN_MOB:
    std::cout << "Received SPAWN_MOB packet" << std::endl;
    break;
```

## Testing Framework Updates

The current testing framework has been updated to:
1. Use correct `MobSpawn` constructor
2. Properly queue spawns using `MapMobs::spawn()`
3. Wait for processing (though actual spawning requires `update()` calls)

However, **actual mob rendering** requires:
- Server connection and packets
- Game loop running `MapMobs::update()`
- Physics system for mob positioning

## Recommendations

1. **For immediate testing**: Create a mock spawn packet system
2. **For development**: Set up a local MapleStory server
3. **For debugging**: Add extensive logging to spawn packet handlers
4. **For testing**: Focus on packet reception and queue processing rather than visual spawning

## Files Modified for Testing
- `Testing/Tests/MobSpawnTest.cpp` - Fixed constructor usage
- `MOB_SPAWN_ANALYSIS.md` - This analysis document
- `test_mob_spawn_packets.cpp` - Connection and packet analysis tool

The mob spawning issue is **not a bug** but rather the **expected behavior** of a client-server architecture where the server controls game state.