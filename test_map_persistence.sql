-- Map Persistence Testing and Validation Queries

-- 1. Before Testing: Check current state
SELECT 'Current Character Map Distribution' as test_type;
SELECT map, COUNT(*) as character_count 
FROM characters 
GROUP BY map 
ORDER BY character_count DESC 
LIMIT 10;

-- 2. Find any remaining invalid map IDs
SELECT 'Invalid Map IDs Found' as test_type;
SELECT id, name, map 
FROM characters 
WHERE map > 999999999 OR map < 0 OR map IN (390625, 67499489);

-- 3. Check for characters with map = 0 (default database value)
SELECT 'Characters with Default Map (0)' as test_type;
SELECT id, name, level, map 
FROM characters 
WHERE map = 0;

-- 4. Verify all characters are in valid map ranges
SELECT 'Map ID Range Validation' as test_type;
SELECT 
    COUNT(*) as total_characters,
    SUM(CASE WHEN map BETWEEN 1 AND 999999999 THEN 1 ELSE 0 END) as valid_range,
    SUM(CASE WHEN map <= 0 THEN 1 ELSE 0 END) as zero_or_negative,
    SUM(CASE WHEN map >= 999999999 THEN 1 ELSE 0 END) as too_large
FROM characters;

-- 5. Check for common valid maps
SELECT 'Common Valid Maps' as test_type;
SELECT 
    map,
    COUNT(*) as count,
    CASE 
        WHEN map = 100000000 THEN 'Henesys'
        WHEN map = 101000000 THEN 'Ellinia'
        WHEN map = 102000000 THEN 'Perion'
        WHEN map = 103000000 THEN 'Kerning City'
        WHEN map = 104000000 THEN 'Lith Harbor'
        WHEN map = 105000000 THEN 'Sleepywood'
        WHEN map = 200000000 THEN 'Orbis'
        WHEN map = 220000000 THEN 'Ludibrium'
        ELSE 'Other'
    END as map_name
FROM characters 
WHERE map IN (100000000, 101000000, 102000000, 103000000, 104000000, 105000000, 200000000, 220000000)
GROUP BY map 
ORDER BY count DESC;

-- 6. Test query: Create a test character with invalid map (for testing)
-- UNCOMMENT TO USE FOR TESTING:
-- INSERT INTO characters (accountid, world, name, level, map, spawnpoint)
-- VALUES (1, 0, 'TestMapPersistence', 1, 390625, 0);

-- 7. Test query: Update a character to invalid map (for testing)
-- UNCOMMENT TO USE FOR TESTING - REPLACE 'YOUR_CHARACTER_NAME' with actual name:
-- UPDATE characters SET map = 390625 WHERE name = 'YOUR_CHARACTER_NAME';

-- 8. Recovery query: Fix any remaining invalid maps
-- UNCOMMENT TO USE IF NEEDED:
-- UPDATE characters 
-- SET map = 100000000 
-- WHERE map > 999999999 OR map < 0 OR map IN (390625, 67499489);

-- 9. Monitoring query: Check for characters that might need attention
SELECT 'Characters That Might Need Attention' as test_type;
SELECT id, name, level, map, spawnpoint
FROM characters
WHERE map NOT IN (
    -- Common town maps
    100000000, 101000000, 102000000, 103000000, 104000000, 105000000,
    110000000, 120000000, 200000000, 220000000, 230000000, 240000000,
    250000000, 260000000, 261000000,
    -- Common training/beginner maps
    0, 1000000, 1000001, 1000002,
    -- Add more valid maps as needed
    60000, 2000000
)
AND map > 0
ORDER BY map;

-- 10. Final validation after fixes
SELECT 'Final Validation Summary' as test_type;
SELECT 
    'Valid Maps' as category,
    COUNT(*) as count
FROM characters 
WHERE map BETWEEN 1 AND 999999999
UNION ALL
SELECT 
    'Invalid Maps' as category,
    COUNT(*) as count
FROM characters 
WHERE map <= 0 OR map >= 999999999
UNION ALL
SELECT 
    'Zero Maps' as category,
    COUNT(*) as count
FROM characters 
WHERE map = 0
UNION ALL
SELECT 
    'Henesys Spawns' as category,
    COUNT(*) as count
FROM characters 
WHERE map = 100000000;