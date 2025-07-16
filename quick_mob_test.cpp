// Quick test to check mob spawning status
// Compile and run this to check current state

#include <iostream>
#include "Net/Session.h"
#include "Gameplay/Stage.h"
#include "Gameplay/MapleMap/MapMobs.h"

using namespace ms;

void quickMobTest() {
    std::cout << "=== QUICK MOB SPAWN TEST ===" << std::endl;
    
    // Check connection
    bool connected = Session::get().is_connected();
    std::cout << "Server connected: " << (connected ? "YES" : "NO") << std::endl;
    
    if (!connected) {
        std::cout << "❌ Not connected to server - connect first!" << std::endl;
        return;
    }
    
    // Check current map
    Stage& stage = Stage::get();
    int32_t mapId = stage.get_mapid();
    std::cout << "Current map ID: " << mapId << std::endl;
    
    if (mapId == 0) {
        std::cout << "❌ No map loaded - load a map first!" << std::endl;
        return;
    }
    
    // Check if we can access MapMobs
    try {
        MapMobs& mobs = stage.get_mobs();
        std::cout << "✅ MapMobs accessible" << std::endl;
    } catch (...) {
        std::cout << "❌ MapMobs not accessible" << std::endl;
        return;
    }
    
    std::cout << "\n=== INSTRUCTIONS ===" << std::endl;
    std::cout << "1. Load Henesys map (100000000)" << std::endl;
    std::cout << "2. Watch console for these messages:" << std::endl;
    std::cout << "   - '[PacketSwitch] SET_FIELD received - starting packet logging'" << std::endl;
    std::cout << "   - '[PacketSwitch] Packet #X: opcode=236 (0xEC) SPAWN_MOB'" << std::endl;
    std::cout << "3. Report what you see in console" << std::endl;
    
    std::cout << "\n=== IF YOU SEE SPAWN_MOB PACKETS ===" << std::endl;
    std::cout << "✅ Server is sending spawn packets correctly" << std::endl;
    std::cout << "🔍 Issue is likely in client-side processing" << std::endl;
    
    std::cout << "\n=== IF YOU DON'T SEE SPAWN_MOB PACKETS ===" << std::endl;
    std::cout << "❌ Server is not sending spawn packets" << std::endl;
    std::cout << "🔍 Check server configuration and map spawn data" << std::endl;
}

int main() {
    quickMobTest();
    
    std::cout << "\nPress Enter to continue..." << std::endl;
    std::cin.get();
    
    return 0;
}