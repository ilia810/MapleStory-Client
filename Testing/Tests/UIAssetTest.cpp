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
#include "../../IO/UI.h"
#include "../../IO/UITypes/UILogin.h"
#include "../../IO/UITypes/UIStatusBar.h"
#include "../../IO/UITypes/UIItemInventory.h"
#include "../../IO/UITypes/UIEquipInventory.h"
#include "../../IO/UITypes/UIMiniMap.h"
#include "../../Graphics/Texture.h"
#include "../../Util/NxFiles.h"

namespace ms {
namespace Testing {

TEST(UIAssets, LoginUIAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    assertNotNull(ui, "UI should not be null");
    
    log("Testing Login UI assets");
    
    ui->emplace<UILogin>();
    headless.waitForUIElement("Login", 2000);
    
    nl::node loginNode = NxFiles::UI()["Login.img"];
    assert(loginNode.size() > 0, "Login.img should have content");
    
    nl::node titleNode = loginNode["Title"];
    assert(titleNode.size() > 0, "Title node should exist");
    
    nl::node commonNode = loginNode["Common"];
    assert(commonNode.size() > 0, "Common node should exist");
    
    nl::node btLoginNode = commonNode["BtLogin"];
    assert(btLoginNode.size() > 0, "BtLogin should exist");
    assert(btLoginNode["normal"]["0"], "BtLogin normal state should exist");
    
    log("Login UI assets loaded successfully");
}

TEST(UIAssets, StatusBarAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    assertNotNull(ui, "UI should not be null");
    
    log("Testing StatusBar UI assets");
    
    ui->emplace<UIStatusBar>(100, 100, 100, 100, 1000, 1);
    headless.waitForUIElement("StatusBar", 2000);
    
    nl::node statusNode = NxFiles::UI()["StatusBar3.img"];
    assert(statusNode.size() > 0, "StatusBar3.img should have content");
    
    nl::node chatTargetNode = statusNode["chat"]["chatTarget"];
    assert(chatTargetNode.size() > 0, "Chat target node should exist");
    
    nl::node menuNode = statusNode["menu"];
    assert(menuNode.size() > 0, "Menu node should exist");
    
    nl::node quickSlotNode = statusNode["quickSlot"];
    assert(quickSlotNode.size() > 0, "QuickSlot node should exist");
    
    log("StatusBar UI assets loaded successfully");
}

TEST(UIAssets, InventoryAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    assertNotNull(ui, "UI should not be null");
    
    log("Testing Inventory UI assets");
    
    ui->emplace<UIItemInventory>(Inventory::Type::EQUIP);
    headless.waitForUIElement("ItemInventory", 2000);
    
    nl::node itemNode = NxFiles::UI()["UIWindow2.img"]["Item"];
    assert(itemNode.size() > 0, "Item node should have content");
    
    nl::node newNode = itemNode["New"];
    assert(newNode.size() > 0, "New node should exist");
    
    nl::node backgrndNode = newNode["backgrnd"];
    assert(backgrndNode, "Background should exist");
    
    nl::node tabNode = newNode["Tab"];
    assert(tabNode.size() > 0, "Tab node should exist");
    
    for (int i = 0; i < 5; i++) {
        nl::node tabEnabledNode = tabNode["enabled"][std::to_string(i)];
        assert(tabEnabledNode, "Tab " + std::to_string(i) + " enabled state should exist");
    }
    
    log("Inventory UI assets loaded successfully");
}

TEST(UIAssets, EquipInventoryAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    assertNotNull(ui, "UI should not be null");
    
    log("Testing Equip Inventory UI assets");
    
    ui->emplace<UIEquipInventory>();
    headless.waitForUIElement("EquipInventory", 2000);
    
    nl::node equipNode = NxFiles::UI()["UIWindow2.img"]["Equip"];
    assert(equipNode.size() > 0, "Equip node should have content");
    
    nl::node characterNode = equipNode["character"];
    assert(characterNode.size() > 0, "Character node should exist");
    
    for (const std::string& slot : {"Weapon", "Cap", "Coat", "Pants", "Shoes", "Glove", "Cape"}) {
        nl::node slotNode = characterNode[slot];
        assert(slotNode, slot + " slot should exist");
    }
    
    log("Equip Inventory UI assets loaded successfully");
}

TEST(UIAssets, MiniMapAssets) {
    HeadlessMode& headless = HeadlessMode::getInstance();
    UI* ui = headless.getUI();
    assertNotNull(ui, "UI should not be null");
    
    log("Testing MiniMap UI assets");
    
    ui->emplace<UIMiniMap>();
    headless.waitForUIElement("MiniMap", 2000);
    
    nl::node minimapNode = NxFiles::UI()["UIWindow2.img"]["MiniMap"];
    assert(minimapNode.size() > 0, "MiniMap node should have content");
    
    nl::node maxMapNode = minimapNode["MaxMap"];
    assert(maxMapNode, "MaxMap should exist");
    
    nl::node minMapNode = minimapNode["MinMap"];
    assert(minMapNode, "MinMap should exist");
    
    nl::node btMapNode = minimapNode["BtMap"];
    assert(btMapNode.size() > 0, "BtMap should exist");
    
    log("MiniMap UI assets loaded successfully");
}

TEST(UIAssets, TextureLoading) {
    log("Testing texture loading from NX files");
    
    nl::node testNode = NxFiles::UI()["Login.img"]["Title"]["logo"];
    assert(testNode, "Logo node should exist");
    
    Texture logoTexture(testNode);
    assert(logoTexture.is_valid(), "Logo texture should be valid");
    assert(logoTexture.get_dimensions().x() > 0, "Logo width should be positive");
    assert(logoTexture.get_dimensions().y() > 0, "Logo height should be positive");
    
    log("Texture dimensions: " + std::to_string(logoTexture.get_dimensions().x()) + 
        "x" + std::to_string(logoTexture.get_dimensions().y()));
    
    log("Texture loading test completed");
}

TEST(UIAssets, AnimationAssets) {
    log("Testing UI animation assets");
    
    nl::node effectNode = NxFiles::UI()["Login.img"]["effect"];
    assert(effectNode.size() > 0, "Effect node should have content");
    
    int frameCount = 0;
    for (auto child : effectNode) {
        frameCount++;
    }
    
    assert(frameCount > 0, "Effect should have animation frames");
    log("Found " + std::to_string(frameCount) + " animation frames");
    
    for (int i = 0; i < std::min(frameCount, 3); i++) {
        nl::node frameNode = effectNode[std::to_string(i)];
        assert(frameNode, "Frame " + std::to_string(i) + " should exist");
    }
    
    log("Animation assets loaded successfully");
}

} // namespace Testing
} // namespace ms