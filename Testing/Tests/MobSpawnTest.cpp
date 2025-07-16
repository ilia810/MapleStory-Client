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
#include "../../Gameplay/MapleMap/Mob.h"
#include "../../Gameplay/Spawn.h"
#include "../../Gameplay/MapleMap/MapMobs.h"

namespace ms {
namespace Testing {

TEST(MobSpawning, SpawnSingleMob) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing single mob spawn");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    int32_t mobId = 100100;
    int32_t oid = 1000;
    Point<int16_t> position(500, 300);
    int8_t stance = 0;
    int16_t fh = 1;
    bool newSpawn = true;
    int8_t team = -1;
    
    // Create MobSpawn struct
    MobSpawn spawn;
    spawn.oid = oid;
    spawn.id = mobId;
    spawn.position = position;
    spawn.stance = stance;
    spawn.fh = fh;
    spawn.newspawn = newSpawn;
    spawn.team = team;
    
    stage->get_mobs().spawn(std::move(spawn));
    
    // Wait a moment for spawn
    headless.waitForCondition([]() { return true; }, 100);
    
    log("Mob spawned successfully");
}

TEST(MobSpawning, SpawnMultipleMobs) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing multiple mob spawns");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    stage->get_mobs().clear();
    
    for (int i = 0; i < 5; i++) {
        MobSpawn spawn;
        spawn.oid = 1000 + i;
        spawn.id = 100100 + i;
        spawn.position = Point<int16_t>(400 + i * 100, 300);
        spawn.stance = 0;
        spawn.fh = 1;
        spawn.newspawn = true;
        spawn.team = -1;
        
        stage->get_mobs().spawn(std::move(spawn));
    }
    
    // Wait a moment for spawns
    headless.waitForCondition([]() { return true; }, 500);
    
    log("All mobs spawned successfully");
}

TEST(MobSpawning, RemoveMob) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob removal");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    MobSpawn spawn;
    spawn.oid = 2000;
    spawn.id = 100100;
    spawn.position = Point<int16_t>(500, 300);
    spawn.stance = 0;
    spawn.fh = 1;
    spawn.newspawn = true;
    spawn.team = -1;
    
    stage->get_mobs().spawn(std::move(spawn));
    
    headless.waitForCondition([]() { return true; }, 100);
    
    stage->get_mobs().remove(2000, 0);
    
    headless.waitForCondition([]() { return true; }, 100);
    
    log("Mob removed successfully");
}

TEST(MobSpawning, MobMovement) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob movement");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    MobSpawn spawn;
    spawn.oid = 3000;
    spawn.id = 100100;
    spawn.position = Point<int16_t>(500, 300);
    spawn.stance = 0;
    spawn.fh = 1;
    spawn.newspawn = true;
    spawn.team = -1;
    
    stage->get_mobs().spawn(std::move(spawn));
    
    headless.waitForCondition([]() { return true; }, 100);
    
    log("Mob movement test completed");
}

TEST(MobSpawning, MobControl) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob control");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    MobSpawn spawn;
    spawn.oid = 4000;
    spawn.id = 100100;
    spawn.position = Point<int16_t>(500, 300);
    spawn.stance = 0;
    spawn.fh = 1;
    spawn.newspawn = true;
    spawn.team = -1;
    
    stage->get_mobs().spawn(std::move(spawn));
    headless.waitForCondition([]() { return true; }, 100);
    
    stage->get_mobs().set_control(4000, true);
    headless.waitForCondition([]() { return true; }, 100);
    
    stage->get_mobs().set_control(4000, false);
    headless.waitForCondition([]() { return true; }, 100);
    
    log("Mob control test completed");
}

TEST(MobSpawning, ClearAllMobs) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing clear all mobs");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    for (int i = 0; i < 3; i++) {
        MobSpawn spawn;
        spawn.oid = 5000 + i;
        spawn.id = 100100;
        spawn.position = Point<int16_t>(400 + i * 100, 300);
        spawn.stance = 0;
        spawn.fh = 1;
        spawn.newspawn = true;
        spawn.team = -1;
        
        stage->get_mobs().spawn(std::move(spawn));
    }
    
    headless.waitForCondition([]() { return true; }, 300);
    
    stage->get_mobs().clear();
    
    headless.waitForCondition([]() { return true; }, 100);
    
    log("All mobs cleared successfully");
}

} // namespace Testing
} // namespace ms