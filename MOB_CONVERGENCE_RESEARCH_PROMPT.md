# Mob Convergence Issue Research Prompt

## Problem Statement
In a MapleStory v83 client-server setup, all mobs are converging to position (0,580) on the client side, despite the server sending varied spawn positions. This is a critical gameplay issue that makes the game unplayable.

## Current Understanding

### Server Behavior (Working Correctly)
1. Server loads mob spawn data from WZ files with correct positions
2. SpawnPoint.getMonster() successfully assigns random positions to mobs when original position is invalid
3. Server logs confirm varied positions being set (e.g., (546,-443), (936,-2085), (-206,277))
4. PacketCreator.spawnMonster() would send these positions in spawn packets
5. The AbstractLoadedLife copy constructor has been fixed to properly copy positions

### Client Behavior (Problematic)
1. All mobs except one converge to position (0,580)
2. One mob (ID: 1000000011) maintains Y=-85 position
3. The Y=580 value matches physics fallback: `borders.second() - 1` when no foothold is found
4. Movement filter for (0,580) was added but doesn't prevent convergence
5. Debug output shows continuous movement to (0,580) with control=1

### Key Code Flow
1. Server: SpawnPoint.getMonster() → Monster.setPosition() → MapleMap.spawnMonster() → PacketCreator.spawnMonster()
2. Client: SpawnMobHandler → MapMobs.add_mob() → MobSpawn.instantiate() → Physics.get_y_below()
3. Client Movement: MobMovedHandler → MapMobs.send_mobmove() → Mob.send_movement()

### Attempted Fixes That Failed
1. Server-side position validation in refreshMobPosition()
2. Server-side random position assignment for invalid spawns
3. Client-side movement packet filtering for (0,580)
4. Server-side AbstractLoadedLife position copying fix

## Investigation Requirements

### Primary Questions
1. Why is the client ignoring the spawn positions sent by the server?
2. What causes all mobs to converge to the same physics fallback position?
3. Why does one mob (1000000011) maintain a different Y position (-85)?
4. Is there a client-side position reset happening after spawn?

### Areas to Investigate
1. **Packet Parsing**: Is SpawnMobHandler correctly reading position data from packets?
2. **Physics System**: Why is get_y_below() being called with (0,y) for all mobs?
3. **Mob State**: Is there a mob state update that resets positions?
4. **Control Flag**: Does control=1 indicate player-controlled movement overriding server positions?
5. **Foothold Data**: Is the client missing foothold data causing physics fallback?

### Debugging Suggestions
1. Add packet hex dump in SpawnMobHandler to verify received data
2. Trace position changes in Mob class from spawn to convergence
3. Check if FootholdTree has valid data for the map
4. Investigate why mobs have control=1 flag
5. Compare working mob (Y=-85) vs converging mobs

### Success Criteria
- Mobs spawn at positions sent by server
- Mobs maintain their positions without converging
- Mobs can move naturally within the map
- No mobs stuck at physics boundaries

## Additional Context
- MapleStory v83 client with custom server (Cosmic)
- Issue occurs on all maps, not map-specific
- Server uses random foothold positions when spawn data is invalid
- Client physics system returns bottom border Y when no foothold found