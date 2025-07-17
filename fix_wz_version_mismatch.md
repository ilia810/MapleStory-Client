# Fix WZ Version Mismatch (v83 Server vs v83/v87 Hybrid Client)

## Problem Identified
- **Server**: Using v83 WZ files (`C:\Users\me\Downloads\PERISH\Cosmic\wz\`)
- **Client**: Configured for v83/v87 hybrid
- **Database**: Contains v87 map IDs that don't exist in v83 WZ files

## Solution Options

### Option 1: Update Server WZ Files (Recommended)
**Pros**: Supports full v83/v87 hybrid functionality
**Cons**: Requires downloading/configuring v87 WZ files

**Steps**:
1. Download v87 or v83/v87 hybrid WZ files
2. Replace server WZ files in `C:\Users\me\Downloads\PERISH\Cosmic\wz\`
3. Restart server
4. Database will work as-is

### Option 2: Clean Database for v83 Compatibility
**Pros**: Quick fix, no file downloads needed
**Cons**: Limits functionality to v83 maps only

**Steps**:
1. Run the SQL script I created: `fix_v83_map_compatibility.sql`
2. This will convert all v87 map IDs to v83 equivalents
3. Restart server

### Option 3: Hybrid Approach
**Configure server to handle missing maps gracefully**

**Steps**:
1. Use the map persistence fixes I implemented (already done)
2. Server will auto-fallback to Henesys for missing maps
3. Database gets auto-corrected over time
4. Upgrade to v87 WZ files when ready

## Quick Fix Commands

### Run Database Cleanup:
```bash
mysql -h localhost -u cosmic -pcosmic123 cosmic < fix_v83_map_compatibility.sql
```

### Check Current Database State:
```sql
SELECT map, COUNT(*) as count FROM characters GROUP BY map ORDER BY count DESC LIMIT 10;
```

### Verify Server WZ Files:
```bash
# Check if server has v87 maps
dir "C:\Users\me\Downloads\PERISH\Cosmic\wz\Map.wz\Map\Map2\"
dir "C:\Users\me\Downloads\PERISH\Cosmic\wz\Map.wz\Map\Map9\"

# v87 maps would be in Map2/ and Map9/ directories
# v83 maps are primarily in Map0/ and Map1/
```

## Expected Results

After applying either fix:
- ✅ Players spawn at correct locations
- ✅ NPCs appear in proper positions
- ✅ No more constant falling
- ✅ Maps load properly
- ✅ No more bunched NPCs

## Testing

1. **Login with a character**
2. **Move to different maps**
3. **Logout and login again**
4. **Verify you spawn at the right location**
5. **Check that NPCs are positioned correctly**

## Which Option to Choose?

**If you want full v83/v87 hybrid functionality**: Use Option 1
**If you want quick fix with v83 only**: Use Option 2  
**If you want to test the fixes first**: Use Option 3

The map persistence fixes I implemented will handle the transition gracefully regardless of which option you choose.