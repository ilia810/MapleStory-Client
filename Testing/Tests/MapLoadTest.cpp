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
#include "../../Gameplay/Stage.h"
#include "../../Gameplay/MapleMap/MapInfo.h"
#include "../../Net/Session.h"
#include "../../Net/Packets/GameplayPackets.h"

namespace ms {
namespace Testing {

TEST(MapLoading, LoadHenesys) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing Henesys map load (100000000)");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    
    headless.waitForMapLoad(100000000, 5000);
    
    assertEqual(100000000, stage->get_mapid(), "Map ID should be Henesys");
    
    const MapInfo* mapinfo = stage->get_map_info();
    assertNotNull(mapinfo, "MapInfo should not be null");
    
    log("Map loaded successfully");
}

TEST(MapLoading, LoadEllinia) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing Ellinia map load (101000000)");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(101000000);
    
    headless.waitForMapLoad(101000000, 5000);
    
    assertEqual(101000000, stage->get_mapid(), "Map ID should be Ellinia");
    
    const MapInfo* mapinfo = stage->get_map_info();
    assertNotNull(mapinfo, "MapInfo should not be null");
    
    log("Map loaded successfully");
}

TEST(MapLoading, LoadKerningCity) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing Kerning City map load (103000000)");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(103000000);
    
    headless.waitForMapLoad(103000000, 5000);
    
    assertEqual(103000000, stage->get_mapid(), "Map ID should be Kerning City");
    
    const MapInfo* mapinfo = stage->get_map_info();
    assertNotNull(mapinfo, "MapInfo should not be null");
    
    log("Map loaded successfully");
}

TEST(MapLoading, TestPortals) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing map portals");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    // Portal testing would require access to MapPortals
    // For now, just verify the map loaded
    log("Portal test completed - map loaded successfully");
}

TEST(MapLoading, TestFootholds) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing map footholds");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    // Physics testing would require access to internal Stage physics
    // For now, just verify the map loaded
    log("Foothold test completed - map loaded successfully");
}

TEST(MapLoading, TestBackgrounds) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing map backgrounds");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    // Background testing would require access to internal Stage backgrounds
    // For now, just verify the map loaded
    log("Map backgrounds test completed - map loaded successfully");
}

TEST(MapLoading, InvalidMapId) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing invalid map ID handling");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    int32_t currentMapId = stage->get_mapid();
    
    stage->loadmap(999999999);
    
    headless.waitForCondition([&stage, currentMapId]() {
        return stage->get_mapid() != currentMapId;
    }, 2000);
    
    assertEqual(currentMapId, stage->get_mapid(), 
        "Map ID should remain unchanged for invalid map");
    
    log("Invalid map ID handled correctly");
}

} // namespace Testing
} // namespace ms