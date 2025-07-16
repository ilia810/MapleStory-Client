#include <iostream>
#include <fstream>
#include "Gameplay/Stage.h"
#include "Gameplay/MapleMap/MapMobs.h"
#include "Gameplay/Spawn.h"
#include "Net/Session.h"
#include "Util/NxFiles.h"
#include "Configuration.h"

using namespace ms;

void test_mob_spawn() {
    std::cout << "=== MOB SPAWN DEBUG TEST ===" << std::endl;
    
    // Check if Stage exists
    Stage& stage = Stage::get();
    std::cout << "Stage instance created" << std::endl;
    
    // Load a test map (Henesys)
    std::cout << "Loading Henesys map (100000000)..." << std::endl;
    stage.loadmap(100000000);
    
    // Wait a moment for map to load
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::cout << "Current map ID: " << stage.get_mapid() << std::endl;
    
    // Check if MapMobs exists
    MapMobs& mobs = stage.get_mobs();
    std::cout << "MapMobs instance obtained" << std::endl;
    
    // Clear existing mobs
    mobs.clear();
    std::cout << "Cleared existing mobs" << std::endl;
    
    // Create a test mob spawn
    std::cout << "Creating test mob spawn..." << std::endl;
    
    // Orange Mushroom (mob ID 100100)
    MobSpawn spawn;
    spawn.oid = 12345;
    spawn.id = 100100;
    spawn.position = Point<int16_t>(500, 300);
    spawn.stance = 0;
    spawn.fh = 1;
    spawn.newspawn = true;
    spawn.team = -1;
    
    std::cout << "Mob spawn created:" << std::endl;
    std::cout << "  OID: " << spawn.oid << std::endl;
    std::cout << "  ID: " << spawn.id << std::endl;
    std::cout << "  Position: (" << spawn.position.x() << ", " << spawn.position.y() << ")" << std::endl;
    std::cout << "  Stance: " << (int)spawn.stance << std::endl;
    std::cout << "  Foothold: " << spawn.fh << std::endl;
    std::cout << "  New spawn: " << spawn.newspawn << std::endl;
    std::cout << "  Team: " << (int)spawn.team << std::endl;
    
    // Try to spawn the mob
    std::cout << "Attempting to spawn mob..." << std::endl;
    try {
        mobs.spawn(std::move(spawn));
        std::cout << "Mob spawn call completed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error during mob spawn: " << e.what() << std::endl;
    }
    
    // Wait a moment for spawn to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "Mob spawn test completed" << std::endl;
    
    // Try to spawn multiple mobs
    std::cout << "\nSpawning multiple test mobs..." << std::endl;
    
    for (int i = 0; i < 5; i++) {
        MobSpawn multiSpawn;
        multiSpawn.oid = 20000 + i;
        multiSpawn.id = 100100 + i;
        multiSpawn.position = Point<int16_t>(400 + i * 100, 350);
        multiSpawn.stance = 0;
        multiSpawn.fh = 1;
        multiSpawn.newspawn = true;
        multiSpawn.team = -1;
        
        try {
            mobs.spawn(std::move(multiSpawn));
            std::cout << "Spawned mob " << i << " (OID: " << (20000 + i) << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error spawning mob " << i << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "Multiple mob spawn test completed" << std::endl;
}

int main() {
    std::cout << "Starting mob spawn debugging..." << std::endl;
    
    // Initialize NX files
    if (Error error = NxFiles::init()) {
        std::cout << "Failed to initialize NX files: " << error.get_message() << std::endl;
        return 1;
    }
    
    std::cout << "NX files initialized successfully" << std::endl;
    
    // Initialize session
    if (Error error = Session::get().init()) {
        std::cout << "Failed to initialize session: " << error.get_message() << std::endl;
        return 1;
    }
    
    std::cout << "Session initialized successfully" << std::endl;
    
    // Run the test
    test_mob_spawn();
    
    std::cout << "\nDebugging complete. Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}