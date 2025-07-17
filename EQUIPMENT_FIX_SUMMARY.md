# Equipment Not Loading Fix

## The Problem
When using auto-login, the player's equipment was not being visually applied to the character, even though the equipment was properly parsed from the server packets.

## The Cause
The SetFieldHandler was parsing the inventory correctly but wasn't applying the equipped items to the character's visual appearance. This step was missing after `recalc_stats()`.

## The Fix
Added code to apply all equipped items to the character's appearance after recalculating stats:

```cpp
// Apply all equipped items to character's visual appearance
LOG(LOG_DEBUG, "[SetFieldHandler] Applying equipped items to character appearance");
for (int16_t slot = 1; slot <= EquipSlot::Id::LENGTH; slot++) {
    player.change_equip(slot);
}
LOG(LOG_DEBUG, "[SetFieldHandler] Equipment applied");
```

This iterates through all equipment slots and calls `player.change_equip(slot)` which:
1. Checks if there's an item in that equipment slot
2. If yes, adds it to the character's visual appearance
3. If no, removes any equipment from that slot

## Location
File: `Net\Handlers\SetFieldHandlers.cpp`
Function: `SetFieldHandler::set_field()`
Added after: `player.recalc_stats(true);`

## Result
Now when using auto-login, the character's equipment will be properly displayed just like when logging in manually.