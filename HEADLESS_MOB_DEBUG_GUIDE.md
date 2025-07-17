# Headless Mob Spawn Debugging Guide

## Overview
Using our headless testing framework to systematically debug the mob spawning issue. This approach gives us controlled, repeatable testing without UI complexity.

## What We Know
✅ Server sends 27 SPAWN_MOB packets (confirmed)
✅ PacketSwitch receives and logs them correctly
❓ Client-side processing needs verification

## Test Suite: MobSpawnDebug

### Quick Start
```bash
# Build and run the comprehensive test
quick_mob_debug.bat

# Or run all debug tests
run_mob_debug_tests.bat
```

### Individual Tests

#### 1. ConnectionStatus
```bash
MapleStory.exe --test-case MobSpawnDebug ConnectionStatus
```
- Checks if connected to server
- Determines if we can test server packets

#### 2. MapLoadedStatus  
```bash
MapleStory.exe --test-case MobSpawnDebug MapLoadedStatus
```
- Verifies map is loaded
- Loads test map if needed

#### 3. ManualSpawnTest
```bash
MapleStory.exe --test-case MobSpawnDebug ManualSpawnTest
```
- Creates manual MobSpawn
- Calls MapMobs::spawn() directly
- **Key Test**: Should show `[DEBUG] MapMobs::spawn() called`

#### 4. PacketHandlerTest
```bash
MapleStory.exe --test-case MobSpawnDebug PacketHandlerTest
```
- Creates mock SPAWN_MOB packet
- Calls SpawnMobHandler::handle() directly
- **Key Test**: Should show `[DEBUG] SpawnMobHandler::handle() called`

#### 5. UpdateLoopTest
```bash
MapleStory.exe --test-case MobSpawnDebug UpdateLoopTest
```
- Adds spawns to queue
- Tests spawn queue behavior
- **Limitation**: Cannot call update() without Physics

#### 6. ServerPacketMonitor
```bash
MapleStory.exe --test-case MobSpawnDebug ServerPacketMonitor
```
- Monitors live server packets
- Calls Session::update() to receive packets
- **Key Test**: Should show PacketSwitch SPAWN_MOB messages

#### 7. FullPipelineTest
```bash
MapleStory.exe --test-case MobSpawnDebug FullPipelineTest
```
- **Most Important Test**: Comprehensive pipeline test
- Tests both server and client-side processing
- Monitors for complete debug message sequence

## Expected Debug Output

### Complete Success Sequence
```
[PacketSwitch] Packet #9: opcode=236 (0xEC) SPAWN_MOB
[DEBUG] SpawnMobHandler::handle() called
[DEBUG] Spawning mob: OID=12345, ID=100100, pos=(500,300), stance=0, fh=1
[DEBUG] MapMobs::spawn() called, adding to queue. Queue size before: 0
[DEBUG] Spawn added to queue. Queue size after: 1
[DEBUG] Mob spawn queued successfully
[DEBUG] MapMobs::update() called, spawns queue size: 1
[DEBUG] Processing spawn from queue: OID=12345
[DEBUG] Creating new mob from spawn
[DEBUG] New mob added to map
```

### Diagnostic by Missing Messages

#### Missing SpawnMobHandler messages
```
[PacketSwitch] Packet #9: opcode=236 (0xEC) SPAWN_MOB
❌ No [DEBUG] SpawnMobHandler::handle() called
```
**Issue**: Packet not being routed to handler

#### Missing MapMobs::spawn messages
```
[DEBUG] SpawnMobHandler::handle() called
❌ No [DEBUG] MapMobs::spawn() called
```
**Issue**: Handler not calling spawn method

#### Missing MapMobs::update messages
```
[DEBUG] MapMobs::spawn() called
❌ No [DEBUG] MapMobs::update() called
```
**Issue**: Game loop not calling update method

#### Queue Never Empties
```
[DEBUG] MapMobs::update() called, spawns queue size: 27
❌ Queue size never decreases
```
**Issue**: Update method not processing spawns

## Test Execution

### Method 1: Quick Test
```bash
quick_mob_debug.bat
```
Runs the most comprehensive test to identify the issue.

### Method 2: Step-by-Step
```bash
run_mob_debug_tests.bat
```
Runs all tests individually for detailed analysis.

### Method 3: Manual Command
```bash
# Build first
powershell -Command "cd 'C:\HeavenClient\MapleStory-Client'; & 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe' MapleStory.vcxproj /p:Configuration=Debug /p:Platform=x64"

# Navigate to exe directory
cd "C:\Users\me\Downloads\PERISH\MapleStory"

# Run specific test
MapleStory.exe --test-case MobSpawnDebug FullPipelineTest
```

## Key Advantages of Headless Testing

1. **Controlled Environment**: No UI interference
2. **Repeatable Tests**: Same conditions every time
3. **Focused Debugging**: Target specific components
4. **Automated Analysis**: Built-in assertions and logging
5. **CI/CD Ready**: Can be automated

## Expected Results

The headless tests will definitively show:
- ✅ Which parts of the pipeline are working
- ❌ Exactly where the pipeline breaks
- 🔍 Detailed debug output for analysis

## Next Steps

1. **Run the tests**: `quick_mob_debug.bat`
2. **Analyze output**: Look for the debug message sequence
3. **Identify break point**: Find where messages stop appearing
4. **Fix the issue**: Target the specific broken component

This systematic approach will solve the mob spawning issue efficiently!