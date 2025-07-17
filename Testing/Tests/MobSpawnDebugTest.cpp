//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.										//
//																				//
//	This program is distributed in the hope that it will be useful,			//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "../TestFramework.h"
#include "../HeadlessMode.h"
#include "../../Net/Session.h"
#include "../../Gameplay/Stage.h"
#include "../../Gameplay/MapleMap/MapMobs.h"
#include "../../Gameplay/Spawn.h"
#include "../../Net/Handlers/MapObjectHandlers.h"
#include "../../Net/InPacket.h"
#include "../../Template/Point.h"
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

namespace ms {
namespace Testing {

TEST(MobSpawnDebug, ConnectionStatus) {
    log("Testing connection status for mob spawning");
    
    Session& session = Session::get();
    bool connected = session.is_connected();
    
    log("Session connected: " + std::string(connected ? "YES" : "NO"));
    
    if (!connected) {
        log("WARNING: Not connected to server - mobs won't spawn from server packets");
        log("This test will focus on client-side processing only");
    }
}

TEST(MobSpawnDebug, MapLoadedStatus) {
    log("Testing map loaded status");
    
    Stage& stage = Stage::get();
    int32_t mapId = stage.get_mapid();
    
    log("Current map ID: " + std::to_string(mapId));
    
    if (mapId == 0) {
        log("WARNING: No map loaded - loading test map");
        stage.loadmap(104000100);
        
        // Wait for map to load
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        mapId = stage.get_mapid();
        log("Map loaded: " + std::to_string(mapId));
    }
    
    assert(mapId != 0, "Map should be loaded");
}

TEST(MobSpawnDebug, ManualSpawnTest) {
    log("Testing manual mob spawn processing");
    
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage& stage = Stage::get();
    
    // Ensure map is loaded
    if (stage.get_mapid() == 0) {
        stage.loadmap(104000100);
        headless.waitForMapLoad(104000100, 5000);
    }
    
    MapMobs& mobs = stage.get_mobs();
    
    // Clear existing spawns
    mobs.clear();
    log("Cleared existing mobs");
    
    // Test 1: Manual spawn creation
    log("Creating manual spawn");
    MobSpawn spawn(12345, 100100, 0, 0, 1, true, -1, Point<int16_t>(500, 300));
    
    log("Adding spawn to queue");
    mobs.spawn(std::move(spawn));
    
    log("Manual spawn test completed - check debug output for MapMobs::spawn() calls");
}

TEST(MobSpawnDebug, PacketHandlerTest) {
    log("Testing SpawnMobHandler packet processing");
    
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage& stage = Stage::get();
    
    // Ensure map is loaded
    if (stage.get_mapid() == 0) {
        stage.loadmap(104000100);
        headless.waitForMapLoad(104000100, 5000);
    }
    
    // Create a mock SPAWN_MOB packet
    log("Creating mock SPAWN_MOB packet");
    
    // Mock packet data for Orange Mushroom spawn
    std::vector<uint8_t> packetData;
    
    // Add OID (4 bytes)
    int32_t oid = 54321;
    packetData.push_back(oid & 0xFF);
    packetData.push_back((oid >> 8) & 0xFF);
    packetData.push_back((oid >> 16) & 0xFF);
    packetData.push_back((oid >> 24) & 0xFF);
    
    // Add controller flag (1 byte)
    packetData.push_back(5);
    
    // Add mob ID (4 bytes)
    int32_t mobId = 100100;
    packetData.push_back(mobId & 0xFF);
    packetData.push_back((mobId >> 8) & 0xFF);
    packetData.push_back((mobId >> 16) & 0xFF);
    packetData.push_back((mobId >> 24) & 0xFF);
    
    // Add 22 bytes of padding
    for (int i = 0; i < 22; i++) {
        packetData.push_back(0);
    }
    
    // Add position (4 bytes)
    int16_t x = 600, y = 400;
    packetData.push_back(x & 0xFF);
    packetData.push_back((x >> 8) & 0xFF);
    packetData.push_back(y & 0xFF);
    packetData.push_back((y >> 8) & 0xFF);
    
    // Add stance (1 byte)
    packetData.push_back(0);
    
    // Add 2 bytes padding
    packetData.push_back(0);
    packetData.push_back(0);
    
    // Add foothold (2 bytes)
    uint16_t fh = 1;
    packetData.push_back(fh & 0xFF);
    packetData.push_back((fh >> 8) & 0xFF);
    
    // Add effect (1 byte)
    packetData.push_back(0);
    
    // Add team (1 byte)
    packetData.push_back(-1);
    
    // Add 4 bytes padding
    for (int i = 0; i < 4; i++) {
        packetData.push_back(0);
    }
    
    // Create InPacket from data
    InPacket packet(packetData.data(), packetData.size());
    
    // Create handler and process packet
    log("Processing mock packet with SpawnMobHandler");
    SpawnMobHandler handler;
    
    try {
        handler.handle(packet);
        log("SpawnMobHandler processed packet successfully");
    } catch (const std::exception& e) {
        log("SpawnMobHandler failed: " + std::string(e.what()));
        fail("SpawnMobHandler should process packet without error");
    }
    
    log("Packet handler test completed - check debug output for handler calls");
}

TEST(MobSpawnDebug, UpdateLoopTest) {
    log("Testing MapMobs::update() loop processing");
    
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage& stage = Stage::get();
    
    // Ensure map is loaded
    if (stage.get_mapid() == 0) {
        stage.loadmap(104000100);
        headless.waitForMapLoad(104000100, 5000);
    }
    
    MapMobs& mobs = stage.get_mobs();
    
    // Clear existing spawns
    mobs.clear();
    
    // Add several test spawns
    log("Adding test spawns to queue");
    for (int i = 0; i < 3; i++) {
        MobSpawn spawn(20000 + i, 100100, 0, 0, 1, true, -1, 
                      Point<int16_t>(400 + i * 100, 300));
        mobs.spawn(std::move(spawn));
    }
    
    // Now we need to manually call update to process spawns
    // Note: This requires Physics parameter, which we don't have access to
    log("Manual update test completed - spawns queued but cannot call update without Physics");
    log("Check debug output for MapMobs::spawn() calls showing queue growth");
}

TEST(MobSpawnDebug, ServerPacketMonitor) {
    log("Testing server packet monitoring");
    
    Session& session = Session::get();
    
    if (!session.is_connected()) {
        log("Not connected to server - skipping packet monitoring");
        skip("Server connection required for packet monitoring");
        return;
    }
    
    HeadlessMode& headless = HeadlessMode::getInstance();
    Stage& stage = Stage::get();
    
    // Load a map if needed
    if (stage.get_mapid() == 0) {
        log("Loading map for packet monitoring");
        stage.loadmap(104000100);
        headless.waitForMapLoad(104000100, 5000);
    }
    
    log("Monitoring for server packets for 5 seconds");
    log("Watch console for PacketSwitch SPAWN_MOB messages");
    
    // Monitor for a short time
    for (int i = 0; i < 50; i++) {
        session.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    log("Packet monitoring completed - check console for SPAWN_MOB packets");
}

TEST(MobSpawnDebug, FullPipelineTest) {
    log("Testing full mob spawn pipeline");
    
    HeadlessMode& headless = HeadlessMode::getInstance();
    Session& session = Session::get();
    Stage& stage = Stage::get();
    
    // Load map if needed
    if (stage.get_mapid() == 0) {
        stage.loadmap(104000100);
        headless.waitForMapLoad(104000100, 5000);
    }
    
    log("Full pipeline test setup complete");
    log("Map ID: " + std::to_string(stage.get_mapid()));
    log("Connected: " + std::string(session.is_connected() ? "YES" : "NO"));
    
    if (session.is_connected()) {
        log("Starting full pipeline test with server connection");
        
        // Monitor for packets and processing
        for (int i = 0; i < 30; i++) {
            session.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        log("Full pipeline test completed");
        log("Expected debug output sequence:");
        log("1. [PacketSwitch] SPAWN_MOB packets");
        log("2. [DEBUG] SpawnMobHandler::handle() called");
        log("3. [DEBUG] MapMobs::spawn() called");
        log("4. [DEBUG] MapMobs::update() called (if game loop runs)");
        
    } else {
        log("No server connection - testing client-side only");
        
        // Test manual spawn
        MapMobs& mobs = stage.get_mobs();
        MobSpawn spawn(99999, 100100, 0, 0, 1, true, -1, Point<int16_t>(500, 300));
        mobs.spawn(std::move(spawn));
        
        log("Manual spawn added - check debug output");
    }
}

} // namespace Testing
} // namespace ms