#include <iostream>
#include <chrono>
#include <thread>
#include "Net/Session.h"
#include "Gameplay/Stage.h"
#include "Gameplay/MapleMap/MapMobs.h"
#include "Util/NxFiles.h"

using namespace ms;

void test_mob_spawn_connection() {
    std::cout << "=== MOB SPAWN PACKET TEST ===" << std::endl;
    
    // Check session connection
    Session& session = Session::get();
    std::cout << "Session connected: " << (session.is_connected() ? "YES" : "NO") << std::endl;
    
    if (!session.is_connected()) {
        std::cout << "ERROR: No server connection - mobs cannot spawn without server packets!" << std::endl;
        std::cout << "Solution: Connect to a MapleStory server to receive mob spawn packets." << std::endl;
        return;
    }
    
    // Check current stage
    Stage& stage = Stage::get();
    std::cout << "Current map ID: " << stage.get_mapid() << std::endl;
    
    if (stage.get_mapid() == 0) {
        std::cout << "ERROR: No map loaded - load a map first!" << std::endl;
        return;
    }
    
    // Monitor for mob spawns over time
    std::cout << "Monitoring mob spawns for 10 seconds..." << std::endl;
    
    MapMobs& mobs = stage.get_mobs();
    
    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Update mobs to process any pending spawns
        // Note: This would normally happen in the main game loop
        // mobs.update(physics); // We can't call this without physics
        
        std::cout << "Second " << (i + 1) << " - Checking for mob updates..." << std::endl;
        
        // Check session for incoming packets
        session.update();
        
        if (!session.is_connected()) {
            std::cout << "Connection lost!" << std::endl;
            break;
        }
    }
    
    std::cout << "Mob spawn monitoring completed." << std::endl;
}

void analyze_mob_spawn_requirements() {
    std::cout << "\n=== MOB SPAWN REQUIREMENTS ANALYSIS ===" << std::endl;
    
    std::cout << "For mobs to spawn in the client, the following is required:" << std::endl;
    std::cout << "1. Connection to a MapleStory server" << std::endl;
    std::cout << "2. Server must send SPAWN_MOB (236) or SPAWN_MOB_C (238) packets" << std::endl;
    std::cout << "3. Client must be in a loaded map" << std::endl;
    std::cout << "4. Game loop must be running to process packets and update mobs" << std::endl;
    
    std::cout << "\nPacket flow for mob spawning:" << std::endl;
    std::cout << "Server -> SPAWN_MOB packet -> SpawnMobHandler -> MapMobs::spawn()" << std::endl;
    std::cout << "-> MobSpawn added to queue -> MapMobs::update() -> Actual mob creation" << std::endl;
    
    std::cout << "\nIf mobs are not spawning, check:" << std::endl;
    std::cout << "- Server connection status" << std::endl;
    std::cout << "- Server is configured to spawn mobs on the current map" << std::endl;
    std::cout << "- Packet handling is working correctly" << std::endl;
    std::cout << "- Game update loop is running" << std::endl;
}

int main() {
    std::cout << "Starting mob spawn packet analysis..." << std::endl;
    
    // Initialize NX files
    if (Error error = NxFiles::init()) {
        std::cout << "Failed to initialize NX files: " << error.get_message() << std::endl;
        return 1;
    }
    
    // Initialize session
    if (Error error = Session::get().init()) {
        std::cout << "Failed to initialize session: " << error.get_message() << std::endl;
        return 1;
    }
    
    // Run connection test
    test_mob_spawn_connection();
    
    // Show analysis
    analyze_mob_spawn_requirements();
    
    std::cout << "\nAnalysis complete. Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}