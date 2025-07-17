#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include "Net/Session.h"
#include "Net/PacketSwitch.h"
#include "Gameplay/Stage.h"
#include "Gameplay/MapleMap/MapMobs.h"
#include "Util/NxFiles.h"
#include "IO/Window.h"
#include "Audio/Audio.h"

using namespace ms;

class MobSpawnDebugger {
public:
    static void logPacket(const std::string& packetName, int opcode) {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        std::cout << "[" << std::setw(8) << millis << "ms] Packet: " << packetName 
                  << " (" << opcode << ")" << std::endl;
    }
    
    static void checkMobSpawnPackets() {
        std::cout << "=== LIVE MOB SPAWN DEBUGGING ===" << std::endl;
        
        // Check session connection
        Session& session = Session::get();
        std::cout << "Session connected: " << (session.is_connected() ? "YES" : "NO") << std::endl;
        
        if (!session.is_connected()) {
            std::cout << "ERROR: Not connected to server!" << std::endl;
            std::cout << "Please connect to the server first, then run this tool." << std::endl;
            return;
        }
        
        // Check current map
        Stage& stage = Stage::get();
        int32_t currentMap = stage.get_mapid();
        std::cout << "Current map ID: " << currentMap << std::endl;
        
        if (currentMap == 0) {
            std::cout << "WARNING: No map loaded. Load a map first." << std::endl;
            return;
        }
        
        // Monitor for 30 seconds
        std::cout << "Monitoring for SPAWN_MOB packets for 30 seconds..." << std::endl;
        std::cout << "Expected packet opcodes: SPAWN_MOB (236), SPAWN_MOB_C (238)" << std::endl;
        std::cout << "Starting monitoring..." << std::endl;
        
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::seconds(30);
        
        int packetCount = 0;
        int spawnPacketCount = 0;
        
        while (std::chrono::steady_clock::now() < endTime) {
            // Update session to receive packets
            session.update();
            
            // Check if still connected
            if (!session.is_connected()) {
                std::cout << "Connection lost during monitoring!" << std::endl;
                break;
            }
            
            // Update stage (this processes mob spawns)
            stage.update();
            
            // Small delay to prevent overwhelming the system
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            packetCount++;
            
            // Every 5 seconds, print status
            if (packetCount % 100 == 0) {
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                std::cout << "[" << seconds << "s] Still monitoring... " << std::endl;
            }
        }
        
        std::cout << "Monitoring completed." << std::endl;
        std::cout << "Total update cycles: " << packetCount << std::endl;
        std::cout << "SPAWN_MOB packets detected: " << spawnPacketCount << std::endl;
        
        // Check mob count after monitoring
        MapMobs& mobs = stage.get_mobs();
        std::cout << "Current mobs in map: [Cannot check directly - no public method]" << std::endl;
    }
    
    static void checkMapMobData() {
        std::cout << "\n=== MAP MOB DATA CHECK ===" << std::endl;
        
        Stage& stage = Stage::get();
        int32_t mapId = stage.get_mapid();
        
        if (mapId == 0) {
            std::cout << "No map loaded." << std::endl;
            return;
        }
        
        // Check NX files for mob data
        std::cout << "Checking NX files for mob data on map " << mapId << "..." << std::endl;
        
        try {
            // Try to access mob data from NX files
            nl::node mapNode = nl::nx::Map["Map"]["Map" + std::to_string(mapId / 100000000)];
            if (mapNode) {
                nl::node mapData = mapNode[std::to_string(mapId) + ".img"];
                if (mapData) {
                    nl::node life = mapData["life"];
                    if (life) {
                        int mobCount = 0;
                        for (auto& lifeNode : life) {
                            std::string type = lifeNode["type"];
                            if (type == "m") { // mob
                                mobCount++;
                                std::string id = lifeNode["id"];
                                std::string x = lifeNode["x"];
                                std::string y = lifeNode["y"];
                                std::cout << "  Mob " << mobCount << ": ID=" << id 
                                         << ", Pos=(" << x << "," << y << ")" << std::endl;
                            }
                        }
                        std::cout << "Total mobs defined in map: " << mobCount << std::endl;
                        
                        if (mobCount == 0) {
                            std::cout << "WARNING: No mobs defined in map data!" << std::endl;
                            std::cout << "This map may not have any mobs to spawn." << std::endl;
                        }
                    } else {
                        std::cout << "No life data found in map." << std::endl;
                    }
                } else {
                    std::cout << "Map data not found in NX files." << std::endl;
                }
            } else {
                std::cout << "Map node not found in NX files." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error checking map data: " << e.what() << std::endl;
        }
    }
    
    static void testManualSpawn() {
        std::cout << "\n=== MANUAL SPAWN TEST ===" << std::endl;
        
        Stage& stage = Stage::get();
        MapMobs& mobs = stage.get_mobs();
        
        std::cout << "Manually creating test mob spawn..." << std::endl;
        
        // Create a test spawn (Orange Mushroom)
        int32_t oid = 99999;
        int32_t mobId = 100100;
        int8_t mode = 0;
        int8_t stance = 0;
        uint16_t fh = 1;
        bool newspawn = true;
        int8_t team = -1;
        Point<int16_t> position(500, 300);
        
        MobSpawn spawn(oid, mobId, mode, stance, fh, newspawn, team, position);
        
        std::cout << "Spawn details:" << std::endl;
        std::cout << "  OID: " << oid << std::endl;
        std::cout << "  Mob ID: " << mobId << " (Orange Mushroom)" << std::endl;
        std::cout << "  Position: (" << position.x() << ", " << position.y() << ")" << std::endl;
        
        // Add spawn to queue
        mobs.spawn(std::move(spawn));
        std::cout << "Spawn added to queue." << std::endl;
        
        // Try to process the spawn
        std::cout << "Processing spawn queue..." << std::endl;
        
        // Note: We can't directly call mobs.update() without physics parameter
        // This would normally be called by the game loop
        std::cout << "NOTE: Actual spawn processing requires physics and game loop." << std::endl;
        std::cout << "Manual spawn test completed." << std::endl;
    }
};

int main() {
    std::cout << "Starting live mob spawn debugging..." << std::endl;
    
    // Initialize systems
    if (Error error = NxFiles::init()) {
        std::cout << "Failed to initialize NX files: " << error.get_message() << std::endl;
        return 1;
    }
    
    if (Error error = Session::get().init()) {
        std::cout << "Failed to initialize session: " << error.get_message() << std::endl;
        return 1;
    }
    
    // Run diagnostics
    MobSpawnDebugger::checkMobSpawnPackets();
    MobSpawnDebugger::checkMapMobData();
    MobSpawnDebugger::testManualSpawn();
    
    std::cout << "\n=== DEBUGGING COMPLETE ===" << std::endl;
    std::cout << "If no SPAWN_MOB packets were detected, possible issues:" << std::endl;
    std::cout << "1. Server is not sending spawn packets for this map" << std::endl;
    std::cout << "2. Map has no mobs defined in its data" << std::endl;
    std::cout << "3. Server configuration issue" << std::endl;
    std::cout << "4. Timing issue - mobs might spawn at specific intervals" << std::endl;
    
    std::cout << "\nPress any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}