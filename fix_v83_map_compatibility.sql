-- Fix v83/v87 Map Compatibility Issues
-- This script cleans up v87 map IDs to v83-compatible ones

-- 1. Show current map distribution
SELECT 'Current Map Distribution' as info;
SELECT map, COUNT(*) as count FROM characters GROUP BY map ORDER BY count DESC LIMIT 20;

-- 2. Find potentially problematic v87 map IDs
SELECT 'Potentially Problematic Map IDs' as info;
SELECT DISTINCT map FROM characters WHERE map > 300000000 AND map < 999999999;

-- 3. Common v83 maps (safe to use)
SELECT 'Valid v83 Maps' as info;
SELECT 'These maps should work with v83 WZ files' as note;

-- 4. Fix known v87 maps to v83 equivalents
UPDATE characters SET map = 100000000 WHERE map = 100000001; -- v87 Henesys variations
UPDATE characters SET map = 101000000 WHERE map = 101000001; -- v87 Ellinia variations
UPDATE characters SET map = 102000000 WHERE map = 102000001; -- v87 Perion variations

-- 5. Fix all maps above v83 range to safe defaults
UPDATE characters SET map = 
    CASE 
        WHEN map BETWEEN 100000000 AND 109999999 THEN 100000000  -- Henesys area → Henesys
        WHEN map BETWEEN 101000000 AND 109999999 THEN 101000000  -- Ellinia area → Ellinia
        WHEN map BETWEEN 102000000 AND 109999999 THEN 102000000  -- Perion area → Perion
        WHEN map BETWEEN 103000000 AND 109999999 THEN 103000000  -- Kerning area → Kerning
        WHEN map BETWEEN 104000000 AND 109999999 THEN 104000000  -- Lith Harbor area → Lith Harbor
        WHEN map BETWEEN 200000000 AND 209999999 THEN 200000000  -- Orbis area → Orbis
        WHEN map BETWEEN 220000000 AND 229999999 THEN 220000000  -- Ludibrium area → Ludibrium
        ELSE 100000000  -- Everything else → Henesys
    END
WHERE map > 300000000 OR map IN (390625, 67499489); -- Fix corrupted IDs

-- 6. Reset all spawn points to 0 (default)
UPDATE characters SET spawnpoint = 0 WHERE spawnpoint > 10;

-- 7. Verify the fix
SELECT 'After Fix - Map Distribution' as info;
SELECT map, COUNT(*) as count FROM characters GROUP BY map ORDER BY count DESC;

-- 8. Final validation
SELECT 'Final Validation' as info;
SELECT 
    COUNT(*) as total_characters,
    SUM(CASE WHEN map BETWEEN 100000000 AND 299999999 THEN 1 ELSE 0 END) as valid_v83_maps,
    SUM(CASE WHEN map >= 300000000 OR map <= 0 THEN 1 ELSE 0 END) as invalid_maps
FROM characters;