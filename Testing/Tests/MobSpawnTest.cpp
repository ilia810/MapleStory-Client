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
#include "../../Gameplay/Mobs/Mob.h"
#include "../../Gameplay/Spawn.h"

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
    
    stage->add_mob(oid, mobId, stance, fh, newSpawn, team, position);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) != nullptr;
    }, 2000);
    
    const Mob* mob = stage->get_mob(oid);
    assertNotNull(mob, "Mob should exist after spawn");
    assertEqual(mobId, mob->get_id(), "Mob ID should match");
    
    log("Mob spawned successfully");
}

TEST(MobSpawning, SpawnMultipleMobs) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing multiple mob spawns");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    stage->clear_mobs();
    
    for (int i = 0; i < 5; i++) {
        int32_t mobId = 100100 + i;
        int32_t oid = 1000 + i;
        Point<int16_t> position(400 + i * 100, 300);
        int8_t stance = 0;
        int16_t fh = 1;
        bool newSpawn = true;
        int8_t team = -1;
        
        stage->add_mob(oid, mobId, stance, fh, newSpawn, team, position);
    }
    
    headless.waitForCondition([&stage]() {
        int count = 0;
        for (int i = 0; i < 5; i++) {
            if (stage->get_mob(1000 + i) != nullptr) {
                count++;
            }
        }
        return count == 5;
    }, 5000);
    
    for (int i = 0; i < 5; i++) {
        const Mob* mob = stage->get_mob(1000 + i);
        assertNotNull(mob, "Mob " + std::to_string(i) + " should exist");
    }
    
    log("All mobs spawned successfully");
}

TEST(MobSpawning, RemoveMob) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob removal");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    int32_t mobId = 100100;
    int32_t oid = 2000;
    Point<int16_t> position(500, 300);
    
    stage->add_mob(oid, mobId, 0, 1, true, -1, position);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) != nullptr;
    }, 2000);
    
    assertNotNull(stage->get_mob(oid), "Mob should exist before removal");
    
    stage->remove_mob(oid, 0);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) == nullptr;
    }, 2000);
    
    assert(stage->get_mob(oid) == nullptr, "Mob should not exist after removal");
    
    log("Mob removed successfully");
}

TEST(MobSpawning, MobMovement) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob movement");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    int32_t mobId = 100100;
    int32_t oid = 3000;
    Point<int16_t> startPos(500, 300);
    
    stage->add_mob(oid, mobId, 0, 1, true, -1, startPos);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) != nullptr;
    }, 2000);
    
    const Mob* mob = stage->get_mob(oid);
    assertNotNull(mob, "Mob should exist");
    
    Point<int16_t> initialPos = mob->get_position();
    log("Initial position: (" + std::to_string(initialPos.x()) + ", " + 
        std::to_string(initialPos.y()) + ")");
    
    for (int i = 0; i < 10; i++) {
        stage->update();
        headless.waitForCondition([]() { return true; }, 100);
    }
    
    Point<int16_t> newPos = mob->get_position();
    log("New position: (" + std::to_string(newPos.x()) + ", " + 
        std::to_string(newPos.y()) + ")");
    
    log("Mob movement test completed");
}

TEST(MobSpawning, MobControl) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    
    log("Testing mob control");
    
    Stage* stage = headless.getStage();
    assertNotNull(stage, "Stage should not be null");
    
    stage->loadmap(100000000);
    headless.waitForMapLoad(100000000, 5000);
    
    int32_t mobId = 100100;
    int32_t oid = 4000;
    Point<int16_t> position(500, 300);
    
    stage->add_mob(oid, mobId, 0, 1, true, -1, position);
    
    headless.waitForCondition([&stage, oid]() {
        return stage->get_mob(oid) != nullptr;
    }, 2000);
    
    stage->set_control(oid, true);
    
    const Mob* mob = stage->get_mob(oid);
    assertNotNull(mob, "Mob should exist");
    assert(mob->is_controlled(), "Mob should be controlled");
    
    stage->set_control(oid, false);
    assert(!mob->is_controlled(), "Mob should not be controlled");
    
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
        int32_t mobId = 100100;
        int32_t oid = 5000 + i;
        Point<int16_t> position(400 + i * 100, 300);
        
        stage->add_mob(oid, mobId, 0, 1, true, -1, position);
    }
    
    headless.waitForCondition([&stage]() {
        return stage->get_mob(5000) != nullptr && 
               stage->get_mob(5001) != nullptr && 
               stage->get_mob(5002) != nullptr;
    }, 3000);
    
    stage->clear_mobs();
    
    headless.waitForCondition([&stage]() {
        return stage->get_mob(5000) == nullptr && 
               stage->get_mob(5001) == nullptr && 
               stage->get_mob(5002) == nullptr;
    }, 2000);
    
    for (int i = 0; i < 3; i++) {
        assert(stage->get_mob(5000 + i) == nullptr, 
            "Mob " + std::to_string(i) + " should not exist after clear");
    }
    
    log("All mobs cleared successfully");
}

} // namespace Testing
} // namespace ms