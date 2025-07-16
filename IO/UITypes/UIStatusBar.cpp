//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "UIStatusBar.h"

#include "UIChannel.h"
#include "UICharInfo.h"
#include "UIChat.h"
#include "UIEquipInventory.h"
#include "UIEvent.h"
#include "UIItemInventory.h"
#include "UIJoypad.h"
#include "UIKeyConfig.h"
#include "UIOptionMenu.h"
#include "UIQuestLog.h"
#include "UIQuit.h"
#include "UISkillBook.h"
#include "UIStatsInfo.h"
#include "UIUserList.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/GameplayPackets.h"
// Console.h not available - removed debug logging

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIStatusBar::UIStatusBar(const CharStats& st) : stats(st)
	{
		quickslot_active = false;
		quickslot_adj = Point<int16_t>(QUICKSLOT_MAX, 0);
		VWIDTH = Constants::Constants::get().get_viewwidth();
		VHEIGHT = Constants::Constants::get().get_viewheight();

		menu_active = false;
		setting_active = false;
		community_active = false;
		character_active = false;
		event_active = false;

		// Detect v87 vs modern client
		bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
		
		std::string stat = "status";

		if (VWIDTH == 800)
			stat += "800";

		// Use appropriate asset paths based on version
		nl::node mainBar;
		nl::node status;
		nl::node EXPBar;
		nl::node EXPBarRes;
		nl::node menu;
		nl::node quickSlot;
		nl::node submenu;
		
		if (!is_v87) {
			mainBar = nl::nx::UI["StatusBar3.img"]["mainBar"];
			status = mainBar[stat];
			EXPBar = mainBar["EXPBar"];
			EXPBarRes = EXPBar[VWIDTH];
			menu = mainBar["menu"];
			quickSlot = mainBar["quickSlot"];
			submenu = mainBar["submenu"];
		} else {
			// V87: Use simplified structure
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			status = statusBar["base"];
			EXPBar = statusBar; // EXP elements are directly under StatusBar
			EXPBarRes = statusBar; // Same for resolution-specific elements
			menu = statusBar; // Menu buttons are directly under StatusBar
			quickSlot = statusBar; // QuickSlot elements too
			submenu = statusBar;
		}

		// Set EXP bar position based on client version
		if (!is_v87) {
			exp_pos = Point<int16_t>(0, 87); // Modern client position
		} else {
			// V87: EXP bar stretches across the bottom of the status bar
			exp_pos = Point<int16_t>(0, 55); // Position at bottom of status bar
		}

		// Only add sprites if the nodes exist
		if (!is_v87) {
			sprites.emplace_back(EXPBar["backgrnd"], DrawArgument(Point<int16_t>(0, 87), Point<int16_t>(VWIDTH, 0)));
			sprites.emplace_back(EXPBarRes["layer:back"], exp_pos);
		} else {
			// V87: Minimal background loading to avoid asset issues
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			
			// Try different possible background locations for v87
			nl::node background;
			if (!statusBar["base"]["backgrnd"].name().empty()) {
				background = statusBar["base"]["backgrnd"];
			} else if (!statusBar["backgrnd"].name().empty()) {
				background = statusBar["backgrnd"];
			}
			
			// Only add background if found
			if (!background.name().empty()) {
				sprites.emplace_back(background, DrawArgument(Point<int16_t>(0, 0), Point<int16_t>(VWIDTH, 0)));
				// V87: Loaded status bar background
			} else {
				// V87: No status bar background found - gauges will render without background
			}
		}

		// Set appropriate width for EXP bar based on client version
		int16_t exp_max;
		if (!is_v87) {
			exp_max = VWIDTH - 16; // Modern client: full width
		} else {
			exp_max = VWIDTH - 20; // V87: full width with small margin
		}

		// Create EXP bar gauge with v87 compatibility
		if (!is_v87) {
			expbar = Gauge(
				Gauge::Type::DEFAULT,
				EXPBarRes.resolve("layer:gauge"),
				EXPBarRes.resolve("layer:cover"),
				EXPBar.resolve("layer:effect"),
				exp_max, 0.0f
			);
		} else {
			// V87: Use dedicated tempExp texture for EXP gauge (yellow-gold color)
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			nl::node gauge = statusBar["gauge"];
			
			// Verify tempExp texture exists
			if (!gauge["tempExp"].name().empty()) {
				Texture expTexture(gauge["tempExp"]);
				expbar = Gauge(Gauge::Type::V87_FILL, expTexture, exp_max, 0.25f); // Test with 25% for visibility
				// Debug: Log successful EXP texture loading
				// V87: Successfully loaded tempExp texture
			} else if (!gauge["bar"].name().empty()) {
				// Fallback: Use the generic bar texture for EXP if tempExp doesn't exist
				Texture barTexture(gauge["bar"]);
				expbar = Gauge(Gauge::Type::V87_FILL, barTexture, exp_max, 0.0f);
				// Debug: Log fallback EXP texture usage
				// V87: Using fallback bar texture for EXP gauge
			} else {
				// Final fallback: Create empty gauge if no textures exist
				expbar = Gauge();
				// Debug: Log missing EXP texture
				// V87: Warning - No EXP gauge texture found, EXP gauge will not render
			}
		}

		int16_t pos_adj = 0;

		if (VWIDTH == 1280)
			pos_adj = 87;
		else if (VWIDTH == 1366)
			pos_adj = 171;
		else if (VWIDTH == 1920)
			pos_adj = 448;

		if (VWIDTH == 1024)
			quickslot_min = 1;
		else
			quickslot_min = 0;

		if (VWIDTH == 800)
		{
			if (!is_v87) {
				hpmp_pos = Point<int16_t>(412, 40);
				mp_pos = hpmp_pos; // Modern client uses same position for HP/MP
				hpset_pos = Point<int16_t>(530, 70);
				mpset_pos = Point<int16_t>(528, 86);
				statset_pos = Point<int16_t>(427, 111);
				levelset_pos = Point<int16_t>(461, 48);
				namelabel_pos = Point<int16_t>(487, 40);
				quickslot_pos = Point<int16_t>(579, 0);
			} else {
				// V87: Different layout with HP/MP on the left, full-width status bar
				hpmp_pos = Point<int16_t>(50, 5);     // HP bar position 
				mp_pos = Point<int16_t>(50, 35);      // MP bar position (30px below HP for very clear separation)
				hpset_pos = Point<int16_t>(200, 25);  // HP value position (right of HP bar)
				mpset_pos = Point<int16_t>(200, 42);  // MP value position (right of MP bar)
				statset_pos = Point<int16_t>(VWIDTH/2, 10); // EXP text position (center top)
				levelset_pos = Point<int16_t>(10, 5);  // Level position (top left)
				namelabel_pos = Point<int16_t>(80, 5); // Character name position (right of level)
				quickslot_pos = Point<int16_t>(VWIDTH - 200, 5); // Quickslot area position (right side)
			}

			// Menu
			menu_pos = Point<int16_t>(682, -280);
			setting_pos = menu_pos + Point<int16_t>(0, 168);
			community_pos = menu_pos + Point<int16_t>(-26, 196);
			character_pos = menu_pos + Point<int16_t>(-61, 168);
			event_pos = menu_pos + Point<int16_t>(-94, 252);
		}
		else
		{
			hpmp_pos = Point<int16_t>(416 + pos_adj, 40);
			mp_pos = hpmp_pos; // Modern client uses same position for HP/MP
			hpset_pos = Point<int16_t>(550 + pos_adj, 70);
			mpset_pos = Point<int16_t>(546 + pos_adj, 86);
			statset_pos = Point<int16_t>(539 + pos_adj, 111);
			levelset_pos = Point<int16_t>(465 + pos_adj, 48);
			namelabel_pos = Point<int16_t>(493 + pos_adj, 40);
			quickslot_pos = Point<int16_t>(628 + pos_adj, 37);

			// Menu
			menu_pos = Point<int16_t>(720 + pos_adj, -280);
			setting_pos = menu_pos + Point<int16_t>(0, 168);
			community_pos = menu_pos + Point<int16_t>(-26, 196);
			character_pos = menu_pos + Point<int16_t>(-61, 168);
			event_pos = menu_pos + Point<int16_t>(-94, 252);
		}

		if (VWIDTH == 1280)
		{
			statset_pos = Point<int16_t>(580 + pos_adj, 111);
			quickslot_pos = Point<int16_t>(622 + pos_adj, 37);

			// Menu
			menu_pos += Point<int16_t>(-7, 0);
			setting_pos += Point<int16_t>(-7, 0);
			community_pos += Point<int16_t>(-7, 0);
			character_pos += Point<int16_t>(-7, 0);
			event_pos += Point<int16_t>(-7, 0);
		}
		else if (VWIDTH == 1366)
		{
			quickslot_pos = Point<int16_t>(623 + pos_adj, 37);

			// Menu
			menu_pos += Point<int16_t>(-5, 0);
			setting_pos += Point<int16_t>(-5, 0);
			community_pos += Point<int16_t>(-5, 0);
			character_pos += Point<int16_t>(-5, 0);
			event_pos += Point<int16_t>(-5, 0);
		}
		else if (VWIDTH == 1920)
		{
			quickslot_pos = Point<int16_t>(900 + pos_adj, 37);

			// Menu
			menu_pos += Point<int16_t>(272, 0);
			setting_pos += Point<int16_t>(272, 0);
			community_pos += Point<int16_t>(272, 0);
			character_pos += Point<int16_t>(272, 0);
			event_pos += Point<int16_t>(272, 0);
		}

		// Only add HP/MP sprites if nodes exist
		if (!is_v87) {
			hpmp_sprites.emplace_back(status["backgrnd"], hpmp_pos - Point<int16_t>(1, 0));
			hpmp_sprites.emplace_back(status["layer:cover"], hpmp_pos - Point<int16_t>(1, 0));

			if (VWIDTH == 800)
				hpmp_sprites.emplace_back(status["layer:Lv"], hpmp_pos);
			else
				hpmp_sprites.emplace_back(status["layer:Lv"], hpmp_pos - Point<int16_t>(1, 0));
		} else {
			// V87: No separate HP/MP background sprites needed
			// The base background is already drawn, gauges render directly over it
			// Leave hpmp_sprites empty for v87
		}

		// V87: Gauge width for v87 should be based on the actual texture size, not stretching
		// Use a more reasonable width for v87 gauges - they should not stretch across the screen
		int16_t hpmp_max;
		if (!is_v87) {
			hpmp_max = (VWIDTH > 800) ? 149 : 120; // Modern client values
		} else {
			hpmp_max = 120; // V87: Use a consistent, reasonable width that matches texture design
		}

		// Create HP/MP gauges with v87 compatibility
		if (!is_v87) {
			hpbar = Gauge(Gauge::Type::DEFAULT, status.resolve("gauge/hp/layer:0"), hpmp_max, 0.0f);
			mpbar = Gauge(Gauge::Type::DEFAULT, status.resolve("gauge/mp/layer:0"), hpmp_max, 0.0f);
		} else {
			// V87: Use correct dedicated gauge textures as identified by researcher
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			nl::node gauge = statusBar["gauge"];
			
			// Verify textures exist before creating gauges
			if (!gauge["hpFlash"]["0"].name().empty() && !gauge["mpFlash"]["0"].name().empty()) {
				// Use frame 0 of hpFlash/mpFlash for static display (avoid blinking)
				Texture hpTexture(gauge["hpFlash"]["0"]);
				Texture mpTexture(gauge["mpFlash"]["0"]);
				// Create gauges using V87_FILL type for proper single-texture rendering
				hpbar = Gauge(Gauge::Type::V87_FILL, hpTexture, hpmp_max, 0.75f); // Test with 75% for visibility
				mpbar = Gauge(Gauge::Type::V87_FILL, mpTexture, hpmp_max, 0.5f);  // Test with 50% for visibility
				// Debug: Log successful texture loading
				// V87: Successfully loaded hpFlash and mpFlash textures
			} else if (!gauge["bar"].name().empty()) {
				// Fallback: Use the generic bar texture for both HP and MP if flash textures don't exist
				Texture barTexture(gauge["bar"]);
				hpbar = Gauge(Gauge::Type::V87_FILL, barTexture, hpmp_max, 0.0f);
				mpbar = Gauge(Gauge::Type::V87_FILL, barTexture, hpmp_max, 0.0f);
				// Debug: Log fallback texture usage
				// V87: Using fallback bar texture for HP/MP gauges
			} else {
				// Final fallback: Create empty gauges if no textures exist
				hpbar = Gauge();
				mpbar = Gauge();
				// Debug: Log missing textures
				// V87: Warning - No gauge textures found, gauges will not render
			}
		}

		// Create character sets with v87 compatibility
		if (!is_v87) {
			statset = Charset(EXPBar["number"], Charset::Alignment::RIGHT);
			hpmpset = Charset(status["gauge"]["number"], Charset::Alignment::RIGHT);
			levelset = Charset(status["lvNumber"], Charset::Alignment::LEFT);
		} else {
			// V87: Use available number textures or create default charsets
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			nl::node numbers = statusBar["number"];
			
			if (!numbers.name().empty()) {
				statset = Charset(numbers, Charset::Alignment::RIGHT);
				hpmpset = Charset(numbers, Charset::Alignment::RIGHT);
				levelset = Charset(numbers, Charset::Alignment::LEFT);
			} else {
				// Create default charsets if no number textures exist
				statset = Charset();
				hpmpset = Charset();
				levelset = Charset();
			}
		}

		namelabel = OutlinedText(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::GALLERY, Color::Name::TUNA);

		// Set quickslot textures with v87 compatibility
		if (!is_v87) {
			quickslot[0] = quickSlot["backgrnd"];
			quickslot[1] = quickSlot["layer:cover"];
		} else {
			// V87: Use empty textures to avoid wrong quickslot textures
			quickslot[0] = Texture();
			quickslot[1] = Texture();
		}

		Point<int16_t> buttonPos = Point<int16_t>(591 + pos_adj, 73);

		if (VWIDTH == 1024)
			buttonPos += Point<int16_t>(38, 0);
		else if (VWIDTH == 1280)
			buttonPos += Point<int16_t>(31, 0);
		else if (VWIDTH == 1366)
			buttonPos += Point<int16_t>(33, 0);
		else if (VWIDTH == 1920)
			buttonPos += Point<int16_t>(310, 0);

		// Only create buttons if the nodes exist
		if (!is_v87) {
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(menu["button:CashShop"], buttonPos);
			buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(menu["button:Menu"], buttonPos);
			buttons[Buttons::BT_OPTIONS] = std::make_unique<MapleButton>(menu["button:Setting"], buttonPos);
			buttons[Buttons::BT_CHARACTER] = std::make_unique<MapleButton>(menu["button:Character"], buttonPos);
			buttons[Buttons::BT_COMMUNITY] = std::make_unique<MapleButton>(menu["button:Community"], buttonPos);
			buttons[Buttons::BT_EVENT] = std::make_unique<MapleButton>(menu["button:Event"], buttonPos);
		} else {
			// V87: Create buttons from available assets with proper spacing
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			// Position buttons with proper spacing on the right side
			Point<int16_t> v87ButtonPos = Point<int16_t>(VWIDTH - 200, 10); // Start further left
			
			if (!statusBar["BtMenu"].name().empty()) {
				buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(statusBar["BtMenu"], v87ButtonPos);
			}
			if (!statusBar["BtShop"].name().empty()) {
				buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(statusBar["BtShop"], v87ButtonPos + Point<int16_t>(60, 0));
			}
			// Add other v87 buttons if they exist
			if (!statusBar["BtClaim"].name().empty()) {
				buttons[Buttons::BT_EVENT] = std::make_unique<MapleButton>(statusBar["BtClaim"], v87ButtonPos + Point<int16_t>(120, 0));
			}
		}

		if (quickslot_active && VWIDTH > 800)
		{
			if (buttons[Buttons::BT_CASHSHOP]) buttons[Buttons::BT_CASHSHOP]->set_active(false);
			if (buttons[Buttons::BT_MENU]) buttons[Buttons::BT_MENU]->set_active(false);
			if (buttons[Buttons::BT_OPTIONS]) buttons[Buttons::BT_OPTIONS]->set_active(false);
			if (buttons[Buttons::BT_CHARACTER]) buttons[Buttons::BT_CHARACTER]->set_active(false);
			if (buttons[Buttons::BT_COMMUNITY]) buttons[Buttons::BT_COMMUNITY]->set_active(false);
			if (buttons[Buttons::BT_EVENT]) buttons[Buttons::BT_EVENT]->set_active(false);
		}

		std::string fold = "button:Fold";
		std::string extend = "button:Extend";

		if (VWIDTH == 800)
		{
			fold += "800";
			extend += "800";
		}

		if (VWIDTH == 1366)
			quickslot_qs_adj = Point<int16_t>(213, 0);
		else
			quickslot_qs_adj = Point<int16_t>(211, 0);

		if (VWIDTH == 800)
		{
			Point<int16_t> quickslot_qs = Point<int16_t>(579, 0);

			// Only create quickslot buttons if nodes exist
			if (!is_v87) {
				buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(quickSlot[fold], quickslot_qs);
				buttons[Buttons::BT_EXTEND_QS] = std::make_unique<MapleButton>(quickSlot[extend], quickslot_qs + quickslot_qs_adj);
			}
		}
		else if (VWIDTH == 1024)
		{
			Point<int16_t> quickslot_qs = Point<int16_t>(627 + pos_adj, 37);

			// Only create quickslot buttons if nodes exist
			if (!is_v87) {
				buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(quickSlot[fold], quickslot_qs);
				buttons[Buttons::BT_EXTEND_QS] = std::make_unique<MapleButton>(quickSlot[extend], quickslot_qs + quickslot_qs_adj);
			}
		}
		else if (VWIDTH == 1280)
		{
			Point<int16_t> quickslot_qs = Point<int16_t>(621 + pos_adj, 37);

			// Only create quickslot buttons if nodes exist
			if (!is_v87) {
				buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(quickSlot[fold], quickslot_qs);
				buttons[Buttons::BT_EXTEND_QS] = std::make_unique<MapleButton>(quickSlot[extend], quickslot_qs + quickslot_qs_adj);
			}
		}
		else if (VWIDTH == 1366)
		{
			Point<int16_t> quickslot_qs = Point<int16_t>(623 + pos_adj, 37);

			// Only create quickslot buttons if nodes exist
			if (!is_v87) {
				buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(quickSlot[fold], quickslot_qs);
				buttons[Buttons::BT_EXTEND_QS] = std::make_unique<MapleButton>(quickSlot[extend], quickslot_qs + quickslot_qs_adj);
			}
		}
		else if (VWIDTH == 1920)
		{
			Point<int16_t> quickslot_qs = Point<int16_t>(900 + pos_adj, 37);

			// Only create quickslot buttons if nodes exist
			if (!is_v87) {
				buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(quickSlot[fold], quickslot_qs);
				buttons[Buttons::BT_EXTEND_QS] = std::make_unique<MapleButton>(quickSlot[extend], quickslot_qs + quickslot_qs_adj);
			}
		}

		// Only manage quickslot button states if buttons exist
		if (!is_v87) {
			if (quickslot_active)
				buttons[Buttons::BT_EXTEND_QS]->set_active(false);
			else
				buttons[Buttons::BT_FOLD_QS]->set_active(false);
		}

#pragma region Menu
		// Set menu backgrounds with v87 compatibility
		if (!is_v87) {
			menubackground[0] = submenu["backgrnd"]["0"];
			menubackground[1] = submenu["backgrnd"]["1"];
			menubackground[2] = submenu["backgrnd"]["2"];
		} else {
			// V87: Create empty textures for menu backgrounds
			menubackground[0] = Texture();
			menubackground[1] = Texture();
			menubackground[2] = Texture();
		}

		// Only create menu buttons if not v87
		if (!is_v87) {
			buttons[Buttons::BT_MENU_ACHIEVEMENT] = std::make_unique<MapleButton>(submenu["menu"]["button:achievement"], menu_pos);
			buttons[Buttons::BT_MENU_AUCTION] = std::make_unique<MapleButton>(submenu["menu"]["button:auction"], menu_pos);
			buttons[Buttons::BT_MENU_BATTLE] = std::make_unique<MapleButton>(submenu["menu"]["button:battleStats"], menu_pos);
			buttons[Buttons::BT_MENU_CLAIM] = std::make_unique<MapleButton>(submenu["menu"]["button:Claim"], menu_pos);
			buttons[Buttons::BT_MENU_FISHING] = std::make_unique<MapleButton>(submenu["menu"]["button:Fishing"], menu_pos + Point<int16_t>(3, 1));
			buttons[Buttons::BT_MENU_HELP] = std::make_unique<MapleButton>(submenu["menu"]["button:Help"], menu_pos);
			buttons[Buttons::BT_MENU_MEDAL] = std::make_unique<MapleButton>(submenu["menu"]["button:medal"], menu_pos);
			buttons[Buttons::BT_MENU_MONSTER_COLLECTION] = std::make_unique<MapleButton>(submenu["menu"]["button:monsterCollection"], menu_pos);
			buttons[Buttons::BT_MENU_MONSTER_LIFE] = std::make_unique<MapleButton>(submenu["menu"]["button:monsterLife"], menu_pos);
			buttons[Buttons::BT_MENU_QUEST] = std::make_unique<MapleButton>(submenu["menu"]["button:quest"], menu_pos);
			buttons[Buttons::BT_MENU_UNION] = std::make_unique<MapleButton>(submenu["menu"]["button:union"], menu_pos);

			buttons[Buttons::BT_SETTING_CHANNEL] = std::make_unique<MapleButton>(submenu["setting"]["button:channel"], setting_pos);
			buttons[Buttons::BT_SETTING_QUIT] = std::make_unique<MapleButton>(submenu["setting"]["button:GameQuit"], setting_pos);
			buttons[Buttons::BT_SETTING_JOYPAD] = std::make_unique<MapleButton>(submenu["setting"]["button:JoyPad"], setting_pos);
			buttons[Buttons::BT_SETTING_KEYS] = std::make_unique<MapleButton>(submenu["setting"]["button:keySetting"], setting_pos);
			buttons[Buttons::BT_SETTING_OPTION] = std::make_unique<MapleButton>(submenu["setting"]["button:option"], setting_pos);

			buttons[Buttons::BT_COMMUNITY_PARTY] = std::make_unique<MapleButton>(submenu["community"]["button:bossParty"], community_pos);
			buttons[Buttons::BT_COMMUNITY_FRIENDS] = std::make_unique<MapleButton>(submenu["community"]["button:friends"], community_pos);
			buttons[Buttons::BT_COMMUNITY_GUILD] = std::make_unique<MapleButton>(submenu["community"]["button:guild"], community_pos);
			buttons[Buttons::BT_COMMUNITY_MAPLECHAT] = std::make_unique<MapleButton>(submenu["community"]["button:mapleChat"], community_pos);

			buttons[Buttons::BT_CHARACTER_INFO] = std::make_unique<MapleButton>(submenu["character"]["button:character"], character_pos);
			buttons[Buttons::BT_CHARACTER_EQUIP] = std::make_unique<MapleButton>(submenu["character"]["button:Equip"], character_pos);
			buttons[Buttons::BT_CHARACTER_ITEM] = std::make_unique<MapleButton>(submenu["character"]["button:Item"], character_pos);
			buttons[Buttons::BT_CHARACTER_SKILL] = std::make_unique<MapleButton>(submenu["character"]["button:Skill"], character_pos);
			buttons[Buttons::BT_CHARACTER_STAT] = std::make_unique<MapleButton>(submenu["character"]["button:Stat"], character_pos);

			buttons[Buttons::BT_EVENT_DAILY] = std::make_unique<MapleButton>(submenu["event"]["button:dailyGift"], event_pos);
			buttons[Buttons::BT_EVENT_SCHEDULE] = std::make_unique<MapleButton>(submenu["event"]["button:schedule"], event_pos);

			for (size_t i = Buttons::BT_MENU_QUEST; i <= Buttons::BT_EVENT_DAILY; i++)
				buttons[i]->set_active(false);

			menutitle[0] = submenu["title"]["character"];
			menutitle[1] = submenu["title"]["community"];
			menutitle[2] = submenu["title"]["event"];
			menutitle[3] = submenu["title"]["menu"];
			menutitle[4] = submenu["title"]["setting"];
		} else {
			// V87: Create empty textures for menu titles
			menutitle[0] = Texture();
			menutitle[1] = Texture();
			menutitle[2] = Texture();
			menutitle[3] = Texture();
			menutitle[4] = Texture();
		}
#pragma endregion

		if (VWIDTH == 800)
		{
			if (!is_v87) {
				position = Point<int16_t>(0, 480);
				position_x = 410;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH - position_x, 140);
			} else {
				// V87: Status bar stretches across bottom of screen - positioned higher
				position = Point<int16_t>(0, VHEIGHT - 75);
				position_x = 0;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH, 75);
			}
		}
		else if (VWIDTH == 1024)
		{
			if (!is_v87) {
				position = Point<int16_t>(0, 648);
				position_x = 410;
				position_y = position.y() + 42;
				dimension = Point<int16_t>(VWIDTH - position_x, 75);
			} else {
				// V87: Status bar stretches across bottom of screen - positioned higher
				position = Point<int16_t>(0, VHEIGHT - 75);
				position_x = 0;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH, 75);
			}
		}
		else if (VWIDTH == 1280)
		{
			if (!is_v87) {
				position = Point<int16_t>(0, 600);
				position_x = 500;
				position_y = position.y() + 42;
				dimension = Point<int16_t>(VWIDTH - position_x, 75);
			} else {
				position = Point<int16_t>(0, VHEIGHT - 80);
				position_x = 0;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH, 80);
			}
		}
		else if (VWIDTH == 1366)
		{
			if (!is_v87) {
				position = Point<int16_t>(0, 648);
				position_x = 585;
				position_y = position.y() + 42;
				dimension = Point<int16_t>(VWIDTH - position_x, 75);
			} else {
				position = Point<int16_t>(0, VHEIGHT - 80);
				position_x = 0;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH, 80);
			}
		}
		else if (VWIDTH == 1920)
		{
			if (!is_v87) {
				position = Point<int16_t>(0, 960 + (VHEIGHT - 1080));
				position_x = 860;
				position_y = position.y() + 40;
				dimension = Point<int16_t>(VWIDTH - position_x, 80);
			} else {
				position = Point<int16_t>(0, VHEIGHT - 80);
				position_x = 0;
				position_y = position.y();
				dimension = Point<int16_t>(VWIDTH, 80);
			}
		}
	}

	void UIStatusBar::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		for (size_t i = 0; i <= Buttons::BT_EVENT; i++)
			if (buttons.find(i) != buttons.end() && buttons.at(i))
				buttons.at(i)->draw(position);

		// Detect v87 vs modern client
		bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();

		// Only draw HP/MP background sprites for modern client
		if (!is_v87) {
			if (hpmp_sprites.size() > 0)
				hpmp_sprites[0].draw(position, alpha);
			if (hpmp_sprites.size() > 1)
				hpmp_sprites[1].draw(position, alpha);
			if (hpmp_sprites.size() > 2)
				hpmp_sprites[2].draw(position, alpha);
		}

		// Draw gauges (both v87 and modern) - only if they're valid
		if (expbar.is_valid())
			expbar.draw(position + exp_pos);
		if (hpbar.is_valid())
			hpbar.draw(position + hpmp_pos);
		if (mpbar.is_valid())
			mpbar.draw(position + mp_pos);

		int16_t level = stats.get_stat(MapleStat::Id::LEVEL);
		int16_t hp = stats.get_stat(MapleStat::Id::HP);
		int16_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);
		int64_t exp = stats.get_exp();

		std::string expstring = std::to_string(100 * getexppercent());

		statset.draw(
			std::to_string(exp) + "[" + expstring.substr(0, expstring.find('.') + 3) + "%]",
			position + statset_pos
		);

		hpmpset.draw(
			"[" + std::to_string(hp) + "/" + std::to_string(maxhp) + "]",
			position + hpset_pos
		);

		hpmpset.draw(
			"[" + std::to_string(mp) + "/" + std::to_string(maxmp) + "]",
			position + mpset_pos
		);

		levelset.draw(
			std::to_string(level),
			position + levelset_pos
		);

		namelabel.draw(position + namelabel_pos);

		// Draw quickslot buttons only if they exist (not v87)
		if (buttons.find(Buttons::BT_FOLD_QS) != buttons.end() && buttons.at(Buttons::BT_FOLD_QS))
			buttons.at(Buttons::BT_FOLD_QS)->draw(position + quickslot_adj);
		if (buttons.find(Buttons::BT_EXTEND_QS) != buttons.end() && buttons.at(Buttons::BT_EXTEND_QS))
			buttons.at(Buttons::BT_EXTEND_QS)->draw(position + quickslot_adj - quickslot_qs_adj);

		if (VWIDTH > 800 && VWIDTH < 1366)
		{
			quickslot[0].draw(position + quickslot_pos + Point<int16_t>(-1, 0) + quickslot_adj);
			quickslot[1].draw(position + quickslot_pos + Point<int16_t>(-1, 0) + quickslot_adj);
		}
		else
		{
			quickslot[0].draw(position + quickslot_pos + quickslot_adj);
			quickslot[1].draw(position + quickslot_pos + quickslot_adj);
		}

#pragma region Menu
		Point<int16_t> pos_adj = Point<int16_t>(0, 0);

		if (quickslot_active)
		{
			if (VWIDTH == 800)
				pos_adj += Point<int16_t>(0, -73);
			else
				pos_adj += Point<int16_t>(0, -31);
		}

		Point<int16_t> pos;
		uint8_t button_count, menutitle_index;

		if (character_active)
		{
			pos = character_pos;
			button_count = 5;
			menutitle_index = 0;
		}
		else if (community_active)
		{
			pos = community_pos;
			button_count = 4;
			menutitle_index = 1;
		}
		else if (event_active)
		{
			pos = event_pos;
			button_count = 2;
			menutitle_index = 2;
		}
		else if (menu_active)
		{
			pos = menu_pos;
			button_count = 11;
			menutitle_index = 3;
		}
		else if (setting_active)
		{
			pos = setting_pos;
			button_count = 5;
			menutitle_index = 4;
		}
		else
		{
			return;
		}

		Point<int16_t> mid_pos = Point<int16_t>(0, 29);

		uint16_t end_y = std::floor(28.2 * button_count);

		if (menu_active)
			end_y -= 1;

		uint16_t mid_y = end_y - mid_pos.y();

		menubackground[0].draw(position + pos + pos_adj);
		menubackground[1].draw(DrawArgument(position + pos + pos_adj) + DrawArgument(mid_pos, Point<int16_t>(0, mid_y)));
		menubackground[2].draw(position + pos + pos_adj + Point<int16_t>(0, end_y));

		menutitle[menutitle_index].draw(position + pos + pos_adj);

		for (size_t i = Buttons::BT_MENU_QUEST; i <= Buttons::BT_EVENT_DAILY; i++)
			if (buttons.find(i) != buttons.end() && buttons.at(i))
				buttons.at(i)->draw(position);
#pragma endregion
	}

	void UIStatusBar::update()
	{
		UIElement::update();

		for (Sprite sprite : hpmp_sprites)
			sprite.update();

		// Only update valid gauges
		if (expbar.is_valid())
			expbar.update(getexppercent());
		if (hpbar.is_valid())
			hpbar.update(gethppercent());
		if (mpbar.is_valid())
			mpbar.update(getmppercent());

		namelabel.change_text(stats.get_name());

		Point<int16_t> pos_adj = get_quickslot_pos();

		if (quickslot_active)
		{
			if (quickslot_adj.x() > quickslot_min)
			{
				int16_t new_x = quickslot_adj.x() - Constants::TIMESTEP;

				if (new_x < quickslot_min)
					quickslot_adj.set_x(quickslot_min);
				else
					quickslot_adj.shift_x(-Constants::TIMESTEP);
			}
		}
		else
		{
			if (quickslot_adj.x() < QUICKSLOT_MAX)
			{
				int16_t new_x = quickslot_adj.x() + Constants::TIMESTEP;

				if (new_x > QUICKSLOT_MAX)
					quickslot_adj.set_x(QUICKSLOT_MAX);
				else
					quickslot_adj.shift_x(Constants::TIMESTEP);
			}
		}

		for (size_t i = Buttons::BT_MENU_QUEST; i <= Buttons::BT_MENU_CLAIM; i++)
		{
			if (buttons.find(i) != buttons.end() && buttons[i])
			{
				Point<int16_t> menu_adj = Point<int16_t>(0, 0);

				if (i == Buttons::BT_MENU_FISHING)
					menu_adj = Point<int16_t>(3, 1);

				buttons[i]->set_position(menu_pos + menu_adj + pos_adj);
			}
		}

		for (size_t i = Buttons::BT_SETTING_CHANNEL; i <= Buttons::BT_SETTING_QUIT; i++)
			if (buttons.find(i) != buttons.end() && buttons[i])
				buttons[i]->set_position(setting_pos + pos_adj);

		for (size_t i = Buttons::BT_COMMUNITY_FRIENDS; i <= Buttons::BT_COMMUNITY_MAPLECHAT; i++)
			if (buttons.find(i) != buttons.end() && buttons[i])
				buttons[i]->set_position(community_pos + pos_adj);

		for (size_t i = Buttons::BT_CHARACTER_INFO; i <= Buttons::BT_CHARACTER_ITEM; i++)
			if (buttons.find(i) != buttons.end() && buttons[i])
				buttons[i]->set_position(character_pos + pos_adj);

		for (size_t i = Buttons::BT_EVENT_SCHEDULE; i <= Buttons::BT_EVENT_DAILY; i++)
			if (buttons.find(i) != buttons.end() && buttons[i])
				buttons[i]->set_position(event_pos + pos_adj);
	}

	Button::State UIStatusBar::button_pressed(uint16_t id)
	{
		switch (id)
		{
			case Buttons::BT_CASHSHOP:
			{
				EnterCashShopPacket().dispatch();
				break;
			}
			case Buttons::BT_MENU:
			{
				toggle_menu();
				break;
			}
			case Buttons::BT_OPTIONS:
			{
				toggle_setting();
				break;
			}
			case Buttons::BT_CHARACTER:
			{
				toggle_character();
				break;
			}
			case Buttons::BT_COMMUNITY:
			{
				toggle_community();
				break;
			}
			case Buttons::BT_EVENT:
			{
				toggle_event();
				break;
			}
			case Buttons::BT_FOLD_QS:
			{
				toggle_qs(false);
				break;
			}
			case Buttons::BT_EXTEND_QS:
			{
				toggle_qs(true);
				break;
			}
			case Buttons::BT_MENU_QUEST:
			{
				UI::get().emplace<UIQuestLog>(
					Stage::get().get_player().get_quests()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_MENU_MEDAL:
			case Buttons::BT_MENU_UNION:
			case Buttons::BT_MENU_MONSTER_COLLECTION:
			case Buttons::BT_MENU_AUCTION:
			case Buttons::BT_MENU_MONSTER_LIFE:
			case Buttons::BT_MENU_BATTLE:
			case Buttons::BT_MENU_ACHIEVEMENT:
			case Buttons::BT_MENU_FISHING:
			case Buttons::BT_MENU_HELP:
			case Buttons::BT_MENU_CLAIM:
			{
				remove_menus();
				break;
			}
			case Buttons::BT_SETTING_CHANNEL:
			{
				UI::get().emplace<UIChannel>();

				remove_menus();
				break;
			}
			case Buttons::BT_SETTING_OPTION:
			{
				UI::get().emplace<UIOptionMenu>();

				remove_menus();
				break;
			}
			case Buttons::BT_SETTING_KEYS:
			{
				UI::get().emplace<UIKeyConfig>(
					Stage::get().get_player().get_inventory(),
					Stage::get().get_player().get_skills()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_SETTING_JOYPAD:
			{
				UI::get().emplace<UIJoypad>();

				remove_menus();
				break;
			}
			case Buttons::BT_SETTING_QUIT:
			{
				UI::get().emplace<UIQuit>(stats);

				remove_menus();
				break;
			}
			case Buttons::BT_COMMUNITY_FRIENDS:
			case Buttons::BT_COMMUNITY_PARTY:
			{
				auto userlist = UI::get().get_element<UIUserList>();
				auto tab = (id == Buttons::BT_COMMUNITY_FRIENDS) ? UIUserList::Tab::FRIEND : UIUserList::Tab::PARTY;

				if (!userlist)
				{
					UI::get().emplace<UIUserList>(tab);
				}
				else
				{
					auto cur_tab = userlist->get_tab();
					auto is_active = userlist->is_active();

					if (cur_tab == tab)
					{
						if (is_active)
							userlist->deactivate();
						else
							userlist->makeactive();
					}
					else
					{
						if (!is_active)
							userlist->makeactive();

						userlist->change_tab(tab);
					}
				}

				remove_menus();
				break;
			}
			case Buttons::BT_COMMUNITY_GUILD:
			{
				remove_menus();
				break;
			}
			case Buttons::BT_COMMUNITY_MAPLECHAT:
			{
				UI::get().emplace<UIChat>();

				remove_menus();
				break;
			}
			case Buttons::BT_CHARACTER_INFO:
			{
				UI::get().emplace<UICharInfo>(
					Stage::get().get_player().get_oid()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_CHARACTER_STAT:
			{
				UI::get().emplace<UIStatsInfo>(
					Stage::get().get_player().get_stats()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_CHARACTER_SKILL:
			{
				UI::get().emplace<UISkillBook>(
					Stage::get().get_player().get_stats(),
					Stage::get().get_player().get_skills()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_CHARACTER_EQUIP:
			{
				UI::get().emplace<UIEquipInventory>(
					Stage::get().get_player().get_inventory()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_CHARACTER_ITEM:
			{
				UI::get().emplace<UIItemInventory>(
					Stage::get().get_player().get_inventory()
					);

				remove_menus();
				break;
			}
			case Buttons::BT_EVENT_SCHEDULE:
			{
				UI::get().emplace<UIEvent>();

				remove_menus();
				break;
			}
			case Buttons::BT_EVENT_DAILY:
			{
				remove_menus();
				break;
			}
		}

		return Button::State::NORMAL;
	}

	void UIStatusBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				if (!menu_active && !setting_active && !community_active && !character_active && !event_active)
					toggle_setting();
				else
					remove_menus();
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				for (size_t i = Buttons::BT_MENU_QUEST; i <= Buttons::BT_EVENT_DAILY; i++)
					if (buttons.find(i) != buttons.end() && buttons[i] && buttons[i]->get_state() == Button::State::MOUSEOVER)
						button_pressed(i);
			}
			else if (keycode == KeyAction::Id::UP || keycode == KeyAction::Id::DOWN)
			{
				uint16_t min_id, max_id;

				if (menu_active)
				{
					min_id = Buttons::BT_MENU_QUEST;
					max_id = Buttons::BT_MENU_CLAIM;
				}
				else if (setting_active)
				{
					min_id = Buttons::BT_SETTING_CHANNEL;
					max_id = Buttons::BT_SETTING_QUIT;
				}
				else if (community_active)
				{
					min_id = Buttons::BT_COMMUNITY_FRIENDS;
					max_id = Buttons::BT_COMMUNITY_MAPLECHAT;
				}
				else if (character_active)
				{
					min_id = Buttons::BT_CHARACTER_INFO;
					max_id = Buttons::BT_CHARACTER_ITEM;
				}
				else if (event_active)
				{
					min_id = Buttons::BT_EVENT_SCHEDULE;
					max_id = Buttons::BT_EVENT_DAILY;
				}

				uint16_t id = min_id;

				for (size_t i = min_id; i <= max_id; i++)
				{
					if (buttons.find(i) != buttons.end() && buttons[i] && buttons[i]->get_state() != Button::State::NORMAL)
					{
						id = i;

						buttons[i]->set_state(Button::State::NORMAL);
						break;
					}
				}

				if (keycode == KeyAction::Id::DOWN)
				{
					if (id < max_id)
						id++;
					else
						id = min_id;
				}
				else if (keycode == KeyAction::Id::UP)
				{
					if (id > min_id)
						id--;
					else
						id = max_id;
				}

				if (buttons.find(id) != buttons.end() && buttons[id])
					buttons[id]->set_state(Button::State::MOUSEOVER);
			}
		}
	}

	bool UIStatusBar::is_in_range(Point<int16_t> cursor_position) const
	{
		Point<int16_t> pos;
		Rectangle<int16_t> bounds;

		if (!character_active && !community_active && !event_active && !menu_active && !setting_active)
		{
			pos = Point<int16_t>(position_x, position_y);
			bounds = Rectangle<int16_t>(pos, pos + dimension);
		}
		else
		{
			uint8_t button_count;
			int16_t pos_y_adj;

			if (character_active)
			{
				pos = character_pos;
				button_count = 5;
				pos_y_adj = 248;
			}
			else if (community_active)
			{
				pos = community_pos;
				button_count = 4;
				pos_y_adj = 301;
			}
			else if (event_active)
			{
				pos = event_pos;
				button_count = 2;
				pos_y_adj = 417;
			}
			else if (menu_active)
			{
				pos = menu_pos;
				button_count = 11;
				pos_y_adj = -90;
			}
			else if (setting_active)
			{
				pos = setting_pos;
				button_count = 5;
				pos_y_adj = 248;
			}

			pos_y_adj += VHEIGHT - 600;

			Point<int16_t> pos_adj = get_quickslot_pos();
			pos = Point<int16_t>(pos.x(), std::abs(pos.y()) + pos_y_adj) + pos_adj;

			uint16_t end_y = std::floor(28.2 * button_count);

			bounds = Rectangle<int16_t>(pos, pos + Point<int16_t>(113, end_y + 35));
		}

		return bounds.contains(cursor_position);
	}

	UIElement::Type UIStatusBar::get_type() const
	{
		return TYPE;
	}

	void UIStatusBar::toggle_qs()
	{
		if (!menu_active && !setting_active && !community_active && !character_active && !event_active)
			toggle_qs(!quickslot_active);
	}

	void UIStatusBar::toggle_qs(bool quick_slot_active)
	{
		if (quickslot_active == quick_slot_active)
			return;

		quickslot_active = quick_slot_active;
		
		// Only manage buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_FOLD_QS) != buttons.end())
			buttons[Buttons::BT_FOLD_QS]->set_active(quickslot_active);
		if (buttons.find(Buttons::BT_EXTEND_QS) != buttons.end())
			buttons[Buttons::BT_EXTEND_QS]->set_active(!quickslot_active);

		if (VWIDTH > 800)
		{
			if (buttons.find(Buttons::BT_CASHSHOP) != buttons.end())
				buttons[Buttons::BT_CASHSHOP]->set_active(!quickslot_active);
			if (buttons.find(Buttons::BT_MENU) != buttons.end())
				buttons[Buttons::BT_MENU]->set_active(!quickslot_active);
			if (buttons.find(Buttons::BT_OPTIONS) != buttons.end())
				buttons[Buttons::BT_OPTIONS]->set_active(!quickslot_active);
			if (buttons.find(Buttons::BT_CHARACTER) != buttons.end())
				buttons[Buttons::BT_CHARACTER]->set_active(!quickslot_active);
			if (buttons.find(Buttons::BT_COMMUNITY) != buttons.end())
				buttons[Buttons::BT_COMMUNITY]->set_active(!quickslot_active);
			if (buttons.find(Buttons::BT_EVENT) != buttons.end())
				buttons[Buttons::BT_EVENT]->set_active(!quickslot_active);
		}
	}

	void UIStatusBar::toggle_menu()
	{
		remove_active_menu(MenuType::MENU);

		menu_active = !menu_active;

		// Only manage menu buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_MENU_ACHIEVEMENT) != buttons.end())
			buttons[Buttons::BT_MENU_ACHIEVEMENT]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_AUCTION) != buttons.end())
			buttons[Buttons::BT_MENU_AUCTION]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_BATTLE) != buttons.end())
			buttons[Buttons::BT_MENU_BATTLE]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_CLAIM) != buttons.end())
			buttons[Buttons::BT_MENU_CLAIM]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_FISHING) != buttons.end())
			buttons[Buttons::BT_MENU_FISHING]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_HELP) != buttons.end())
			buttons[Buttons::BT_MENU_HELP]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_MEDAL) != buttons.end())
			buttons[Buttons::BT_MENU_MEDAL]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_MONSTER_COLLECTION) != buttons.end())
			buttons[Buttons::BT_MENU_MONSTER_COLLECTION]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_MONSTER_LIFE) != buttons.end())
			buttons[Buttons::BT_MENU_MONSTER_LIFE]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_QUEST) != buttons.end())
			buttons[Buttons::BT_MENU_QUEST]->set_active(menu_active);
		if (buttons.find(Buttons::BT_MENU_UNION) != buttons.end())
			buttons[Buttons::BT_MENU_UNION]->set_active(menu_active);

		if (menu_active)
		{
			if (buttons.find(Buttons::BT_MENU_QUEST) != buttons.end())
				buttons[Buttons::BT_MENU_QUEST]->set_state(Button::State::MOUSEOVER);

			Sound(Sound::Name::DLGNOTICE).play();
		}
	}

	void UIStatusBar::toggle_setting()
	{
		remove_active_menu(MenuType::SETTING);

		setting_active = !setting_active;

		// Only manage setting buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_SETTING_CHANNEL) != buttons.end())
			buttons[Buttons::BT_SETTING_CHANNEL]->set_active(setting_active);
		if (buttons.find(Buttons::BT_SETTING_QUIT) != buttons.end())
			buttons[Buttons::BT_SETTING_QUIT]->set_active(setting_active);
		if (buttons.find(Buttons::BT_SETTING_JOYPAD) != buttons.end())
			buttons[Buttons::BT_SETTING_JOYPAD]->set_active(setting_active);
		if (buttons.find(Buttons::BT_SETTING_KEYS) != buttons.end())
			buttons[Buttons::BT_SETTING_KEYS]->set_active(setting_active);
		if (buttons.find(Buttons::BT_SETTING_OPTION) != buttons.end())
			buttons[Buttons::BT_SETTING_OPTION]->set_active(setting_active);

		if (setting_active)
		{
			if (buttons.find(Buttons::BT_SETTING_CHANNEL) != buttons.end())
				buttons[Buttons::BT_SETTING_CHANNEL]->set_state(Button::State::MOUSEOVER);

			Sound(Sound::Name::DLGNOTICE).play();
		}
	}

	void UIStatusBar::toggle_community()
	{
		remove_active_menu(MenuType::COMMUNITY);

		community_active = !community_active;

		// Only manage community buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_COMMUNITY_PARTY) != buttons.end())
			buttons[Buttons::BT_COMMUNITY_PARTY]->set_active(community_active);
		if (buttons.find(Buttons::BT_COMMUNITY_FRIENDS) != buttons.end())
			buttons[Buttons::BT_COMMUNITY_FRIENDS]->set_active(community_active);
		if (buttons.find(Buttons::BT_COMMUNITY_GUILD) != buttons.end())
			buttons[Buttons::BT_COMMUNITY_GUILD]->set_active(community_active);
		if (buttons.find(Buttons::BT_COMMUNITY_MAPLECHAT) != buttons.end())
			buttons[Buttons::BT_COMMUNITY_MAPLECHAT]->set_active(community_active);

		if (community_active)
		{
			if (buttons.find(Buttons::BT_COMMUNITY_FRIENDS) != buttons.end())
				buttons[Buttons::BT_COMMUNITY_FRIENDS]->set_state(Button::State::MOUSEOVER);

			Sound(Sound::Name::DLGNOTICE).play();
		}
	}

	void UIStatusBar::toggle_character()
	{
		remove_active_menu(MenuType::CHARACTER);

		character_active = !character_active;

		// Only manage character buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_CHARACTER_INFO) != buttons.end())
			buttons[Buttons::BT_CHARACTER_INFO]->set_active(character_active);
		if (buttons.find(Buttons::BT_CHARACTER_EQUIP) != buttons.end())
			buttons[Buttons::BT_CHARACTER_EQUIP]->set_active(character_active);
		if (buttons.find(Buttons::BT_CHARACTER_ITEM) != buttons.end())
			buttons[Buttons::BT_CHARACTER_ITEM]->set_active(character_active);
		if (buttons.find(Buttons::BT_CHARACTER_SKILL) != buttons.end())
			buttons[Buttons::BT_CHARACTER_SKILL]->set_active(character_active);
		if (buttons.find(Buttons::BT_CHARACTER_STAT) != buttons.end())
			buttons[Buttons::BT_CHARACTER_STAT]->set_active(character_active);

		if (character_active)
		{
			if (buttons.find(Buttons::BT_CHARACTER_INFO) != buttons.end())
				buttons[Buttons::BT_CHARACTER_INFO]->set_state(Button::State::MOUSEOVER);

			Sound(Sound::Name::DLGNOTICE).play();
		}
	}

	void UIStatusBar::toggle_event()
	{
		remove_active_menu(MenuType::EVENT);

		event_active = !event_active;

		// Only manage event buttons if they exist (not v87)
		if (buttons.find(Buttons::BT_EVENT_DAILY) != buttons.end())
			buttons[Buttons::BT_EVENT_DAILY]->set_active(event_active);
		if (buttons.find(Buttons::BT_EVENT_SCHEDULE) != buttons.end())
			buttons[Buttons::BT_EVENT_SCHEDULE]->set_active(event_active);

		if (event_active)
		{
			if (buttons.find(Buttons::BT_EVENT_SCHEDULE) != buttons.end())
				buttons[Buttons::BT_EVENT_SCHEDULE]->set_state(Button::State::MOUSEOVER);

			Sound(Sound::Name::DLGNOTICE).play();
		}
	}

	void UIStatusBar::remove_menus()
	{
		if (menu_active)
			toggle_menu();
		else if (setting_active)
			toggle_setting();
		else if (community_active)
			toggle_community();
		else if (character_active)
			toggle_character();
		else if (event_active)
			toggle_event();
	}

	void UIStatusBar::remove_active_menu(MenuType type)
	{
		for (size_t i = Buttons::BT_MENU_QUEST; i <= Buttons::BT_EVENT_DAILY; i++)
			if (buttons.find(i) != buttons.end() && buttons[i])
				buttons[i]->set_state(Button::State::NORMAL);

		if (menu_active && type != MenuType::MENU)
			toggle_menu();
		else if (setting_active && type != MenuType::SETTING)
			toggle_setting();
		else if (community_active && type != MenuType::COMMUNITY)
			toggle_community();
		else if (character_active && type != MenuType::CHARACTER)
			toggle_character();
		else if (event_active && type != MenuType::EVENT)
			toggle_event();
	}

	Point<int16_t> UIStatusBar::get_quickslot_pos() const
	{
		if (quickslot_active)
		{
			if (VWIDTH == 800)
				return Point<int16_t>(0, -73);
			else
				return Point<int16_t>(0, -31);
		}

		return Point<int16_t>(0, 0);
	}

	bool UIStatusBar::is_menu_active()
	{
		return menu_active || setting_active || community_active || character_active || event_active;
	}

	float UIStatusBar::getexppercent() const
	{
		int16_t level = stats.get_stat(MapleStat::Id::LEVEL);

		if (level >= ExpTable::LEVELCAP)
			return 0.0f;

		int64_t exp = stats.get_exp();

		return static_cast<float>(
			static_cast<double>(exp) / ExpTable::values[level]
			);
	}

	float UIStatusBar::gethppercent() const
	{
		int16_t hp = stats.get_stat(MapleStat::Id::HP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);

		return static_cast<float>(hp) / maxhp;
	}

	float UIStatusBar::getmppercent() const
	{
		int16_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);

		return static_cast<float>(mp) / maxmp;
	}
}