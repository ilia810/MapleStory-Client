# MapleStory Client - Mob Convergence Issue Repomix

## Issue Summary
All mobs are converging to position (0,580) despite server sending varied spawn positions. This suggests a client-side physics or spawn handling issue.

## Key Files

### Gameplay/Spawn.cpp
```cpp
std::unique_ptr<MapObject> MobSpawn::instantiate(const Physics& physics) const
{
    Point<int16_t> spawnposition;
    
    // Get physics borders for debugging
    auto borders = physics.get_fht().get_borders();
    
    // Always use physics to find proper ground position
    // This ensures mobs spawn on valid footholds
    spawnposition = physics.get_y_below(position);
    
    printf("[MobSpawn] Spawn mob %d: input pos=(%d,%d) -> ground pos=(%d,%d), fh=%d, borders=(%d,%d)\n", 
        oid, position.x(), position.y(), spawnposition.x(), spawnposition.y(), fh, 
        borders.first(), borders.second());
    
    // Check if physics returned the bottom border (no foothold found)
    if (spawnposition.y() == borders.second() - 1) {
        printf("[MobSpawn] WARNING: No foothold found for mob %d at X=%d, physics returned bottom border Y=%d\n", 
            oid, position.x(), spawnposition.y());
        // Try to use the original position or foothold info
        if (fh > 0) {
            // We have a foothold ID, use original position
            spawnposition = position;
            printf("[MobSpawn] Using original position with foothold %d\n", fh);
        }
    }
    
    return std::make_unique<Mob>(oid, id, mode, stance, fh, newspawn, team, spawnposition);
}
```

### Gameplay/MapleMap/Mob.cpp
```cpp
void Mob::send_movement(Point<int16_t> start, std::vector<Movement>&& in_movements)
{
    if (control)
        return;

    // CRITICAL FIX: Ignore movement packets that try to set position to (0,580)
    // This is a convergence bug from the server
    if (start.x() == 0 && start.y() == 580) {
        printf("[Mob::send_movement] WARNING: Ignoring movement to (0,580) for mob %d\n", oid);
        return;
    }

    set_position(start.x(), start.y());
    
    // ... rest of function
}
```

### Gameplay/Physics/Physics.cpp
```cpp
Point<int16_t> Physics::get_y_below(Point<int16_t> position) const
{
    if (auto ground = fht.get_ground_below(position))
    {
        return { position.x(), ground };
    }
    else
    {
        auto borders = fht.get_borders();
        return { position.x(), static_cast<int16_t>(borders.second() - 1) };
    }
}
```

### Net/Handlers/MapObjectHandlers.cpp
```cpp
void SpawnMobHandler::handle(InPacket& recv) const
{
    int32_t oid = recv.read_int();
    uint8_t mode = recv.read_byte();
    int32_t id = recv.read_int();
    recv.skip(22); // Skip mob stats

    Point<int16_t> position = recv.read_point();
    int8_t stance = recv.read_byte();
    
    recv.skip(2); // Skip origin FH
    int16_t fh = recv.read_short();
    
    // Debug logging
    printf("[SpawnMobHandler] Received spawn for mob %d (OID: %d) at pos (%d,%d) with fh=%d\n", 
           id, oid, position.x(), position.y(), fh);
    
    // Check for invalid spawn position
    if (position.x() == 0 && position.y() == 0) {
        printf("[SpawnMobHandler] WARNING: Server sent invalid spawn position (0,0) for mob %d\n", id);
    }
    
    int8_t team = recv.read_byte();
    recv.skip(4);

    Stage::get().get_mobs().add_mob(
        oid,
        id,
        mode,
        stance,
        fh,
        recv.read_bool(),
        team,
        position
    );
}

void MobMovedHandler::handle(InPacket& recv) const
{
    int32_t oid = recv.read_int();
    recv.skip(1);
    uint8_t mode = recv.read_byte();
    recv.skip(1);
    uint8_t action = recv.read_byte();
    recv.skip(2);
    
    Point<int16_t> position = recv.read_point();
    
    // Debug logging for problematic position
    if (position.x() == 0 && position.y() == 580) {
        printf("[MobMovedHandler] WARNING: Received movement to (0,580) for mob OID %d\n", oid);
    }
    
    std::vector<Movement> movements;
    uint8_t movecount = recv.read_byte();
    for (uint8_t i = 0; i < movecount; i++)
    {
        movements.push_back(recv.read_movement();
    }
    
    Stage::get().get_mobs().send_mobmove(oid, position, std::move(movements));
}
```

### Gameplay/MapleMap/MapMobs.cpp
```cpp
void MapMobs::add_mob(int32_t oid, int32_t id, int8_t mode, int8_t stance, int16_t fhid, 
                      bool newspawn, int8_t team, Point<int16_t> position)
{
    if (mobs.count(oid))
        return;

    printf("[MapMobs::add_mob] Adding mob %d (OID: %d) at position (%d,%d) with fh=%d\n", 
           id, oid, position.x(), position.y(), fhid);

    auto spawn = MobSpawn(oid, id, mode, stance, fhid, newspawn, team, position);
    auto mob = spawn.instantiate(Stage::get().get_physics());
    
    mobs[oid] = std::move(mob);
}

void MapMobs::send_mobmove(int32_t oid, Point<int16_t> start, std::vector<Movement> movements)
{
    if (auto* mob = get_mob(oid))
    {
        // Debug logging
        printf("[MapMobs::send_mobmove] Mob %d moving from (%d,%d)\n", oid, start.x(), start.y());
        
        mob->send_movement(start, std::move(movements));
    }
}
```

## Debug Output Pattern
```
[DEBUG] Mob 1000000011 at unusual Y position: -85
[DEBUG] Mob 1000000020 moving at pos (0,580), stance=2, control=1
[DEBUG] Mob 1000000032 moving at pos (0,580), stance=2, control=1
[DEBUG] Mob 1000000021 moving at pos (0,580), stance=2, control=1
```

## Key Observations
1. One mob (1000000011) maintains an unusual Y position of -85
2. All other mobs converge to (0,580)
3. The Y=580 suggests physics fallback when no foothold is found
4. Movement filter for (0,580) is not preventing the convergence