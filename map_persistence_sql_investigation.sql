-- Map Persistence Investigation Queries

-- 1. Check current character locations
SELECT id, name, level, map, spawnpoint 
FROM characters 
ORDER BY id DESC 
LIMIT 20;

-- 2. Find all unique map IDs in use
SELECT DISTINCT map, COUNT(*) as player_count 
FROM characters 
GROUP BY map 
ORDER BY player_count DESC;

-- 3. Find potentially corrupted map IDs
SELECT id, name, map 
FROM characters 
WHERE map > 999999999 
   OR map < 0 
   OR map IN (390625, 67499489);

-- 4. Check recent character modifications (if your table has timestamps)
-- Adjust column names based on your schema
-- SELECT id, name, map, last_modified 
-- FROM characters 
-- ORDER BY last_modified DESC 
-- LIMIT 10;

-- 5. Find characters stuck in non-town maps
SELECT id, name, map 
FROM characters 
WHERE map NOT IN (
    100000000, 101000000, 102000000, 103000000, 104000000,
    105000000, 110000000, 120000000, 200000000, 220000000,
    230000000, 240000000, 250000000, 260000000, 261000000,
    60000, 2000000
) 
AND map != 0
ORDER BY map;

-- 6. Emergency fix - Reset all invalid maps to Henesys
-- UNCOMMENT TO USE:
-- UPDATE characters 
-- SET map = 100000000 
-- WHERE map > 999999999 
--    OR map < 0 
--    OR map IN (390625, 67499489);

-- 7. Create audit log table to track map changes (optional)
-- CREATE TABLE IF NOT EXISTS character_map_log (
--     id INT AUTO_INCREMENT PRIMARY KEY,
--     character_id INT,
--     old_map INT,
--     new_map INT,
--     change_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
--     INDEX idx_character (character_id),
--     INDEX idx_time (change_time)
-- );

-- 8. Check if any accounts have multiple characters in different maps
SELECT 
    a.id as account_id,
    GROUP_CONCAT(CONCAT(c.name, ':', c.map) SEPARATOR ', ') as characters_maps
FROM accounts a
JOIN characters c ON a.id = c.accountid
GROUP BY a.id
HAVING COUNT(DISTINCT c.map) > 1;