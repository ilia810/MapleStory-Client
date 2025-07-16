# Live Server Mob Spawn Debugging Guide

Since your server is running, let's debug exactly why mobs aren't spawning. The client already has built-in packet logging that activates automatically.

## Step 1: Enable Console Output

The packet logging already exists in `PacketSwitch.cpp` and automatically enables when you enter a map. It logs the first 50 packets, including SPAWN_MOB packets.

**What to look for:**
- `[PacketSwitch] SET_FIELD received - starting packet logging`
- `[PacketSwitch] Packet #X: opcode=236 (0xEC) SPAWN_MOB`
- `[PacketSwitch] Packet #X: opcode=238 (0xEE) SPAWN_MOB_C`

## Step 2: Test Procedure

1. **Run the client** and connect to your server
2. **Load a map** (like Henesys - 100000000)
3. **Watch the console output** for packet messages
4. **Look specifically for SPAWN_MOB packets**

## Step 3: Interpret Results

### ✅ If you see SPAWN_MOB packets:
```
[PacketSwitch] Packet #5: opcode=236 (0xEC) SPAWN_MOB
[PacketSwitch] Packet #8: opcode=236 (0xEC) SPAWN_MOB
```

**This means:** Server is sending spawn packets but mobs aren't appearing visually.

**Possible issues:**
- Game loop not calling `MapMobs::update()`
- Rendering issue
- Physics not initialized
- Mob assets not loading

### ❌ If you DON'T see SPAWN_MOB packets:
```
[PacketSwitch] SET_FIELD received - starting packet logging
[PacketSwitch] Packet #1: opcode=125 (0x7D) SET_FIELD
[PacketSwitch] Packet #2: opcode=257 (0x101) SPAWN_NPC
[PacketSwitch] Packet #3: opcode=268 (0x10C) DROP_LOOT
... no SPAWN_MOB packets ...
```

**This means:** Server is not sending spawn packets for this map.

**Possible server issues:**
- Map has no mobs configured
- Server spawn system disabled
- Map ID mismatch
- Database/spawn configuration issue

## Step 4: Server-Side Debugging

If no SPAWN_MOB packets are received, check your server:

### Check Server Console/Logs
Look for mob spawn related messages in your server logs.

### Check Map Configuration
Verify your server has mob spawn data for the current map:
- Check database tables (usually `spawns` or `life`)
- Verify map ID matches what client is loading
- Check if mobs are enabled for that map

### Common Server Issues
1. **Empty spawn tables** - No mobs configured for map
2. **Spawn rate set to 0** - Mobs configured but not spawning
3. **Map ID mismatch** - Server using different map numbering
4. **Spawn system disabled** - Server not running spawn threads

## Step 5: Manual Testing

If you want to manually test mob spawning in the client:

```cpp
// Add this to your main game loop or create a test function
void testManualMobSpawn() {
    Stage& stage = Stage::get();
    if (stage.get_mapid() == 0) return;
    
    // Create test spawn
    MobSpawn spawn(99999, 100100, 0, 0, 1, true, -1, Point<int16_t>(500, 300));
    stage.get_mobs().spawn(std::move(spawn));
    
    // Note: This adds to queue, still needs MapMobs::update() to process
}
```

## Step 6: Check Game Loop

The most likely issue if packets are received but mobs don't appear is that `MapMobs::update()` isn't being called. This should happen in the main game loop.

**Check in `MapleStory.cpp` main loop:**
```cpp
// Should contain something like:
stage.update();  // This calls MapMobs::update()
```

## Step 7: Quick Fixes

### If packets are received but mobs don't appear:
1. Verify `Stage::update()` is called in main loop
2. Check if physics is initialized
3. Ensure rendering system is working

### If no packets are received:
1. Check server spawn configuration
2. Verify map has mobs in database
3. Check server console for errors
4. Try a different map known to have mobs

## Expected Behavior

When working correctly, you should see:
1. `SET_FIELD` packet when entering map
2. Multiple `SPAWN_MOB` packets shortly after
3. Mobs appearing visually in the game

## Next Steps

**Run the client now and:**
1. Connect to your server
2. Load Henesys (map 100000000)
3. Watch console for SPAWN_MOB packets
4. Report back what you see

This will tell us exactly where the issue is in the mob spawning pipeline.