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

		// v92: Use StatusBar.img structure
		nl::node statusBar = nl::nx::UI["StatusBar.img"];
		nl::node base = statusBar["base"];
		nl::node gauge = statusBar["gauge"];
		
		if (!statusBar || !base || !gauge) {
			// StatusBar.img assets not found - create with defaults
			return;
		}
		
		// v92: EXP bar stretches across the bottom of the status bar
		exp_pos = Point<int16_t>(0, 55); // Position at bottom of status bar

		// v92: Load status bar background components
		// Add main background if found - v92 uses canvas type for textures
		nl::node background = base["backgrnd"];
		if (background) {
			sprites.emplace_back(background, DrawArgument(Point<int16_t>(0, 0)));
		}
		
		// Add backgrnd2 - secondary background element
		nl::node background2 = base["backgrnd2"];
		if (background2) {
			sprites.emplace_back(background2, DrawArgument(Point<int16_t>(0, 0)));
		}

		// Add gauge.bar - the bar texture that goes under graduation
		nl::node bar = gauge["bar"];
		if (bar) {
			sprites.emplace_back(bar, Point<int16_t>(215, 35)); // Same position as graduation
		}
		// Add gauge.graduation - shows the empty bar outlines for HP/MP/XP (positioned lower)
		nl::node graduation = gauge["graduation"];
		if (graduation) {
			sprites.emplace_back(graduation, Point<int16_t>(215, 35)); // Position it lower
		}
		
		// Add base.chat - chat interface element
		nl::node chat = base["chat"];
		if (chat) {
			sprites.emplace_back(chat, DrawArgument(Point<int16_t>(0, 0)));
		}

	
		int16_t exp_max = VWIDTH - 20; 

		// v92: Use dedicated tempExp texture for EXP gauge (yellow-gold color)
		// Verify tempExp texture exists
		if (gauge["tempExp"]) {
			Texture expTexture(gauge["tempExp"]);
			expbar = Gauge(Gauge::Type::V87_FILL, expTexture, exp_max, 0.25f); // Test with 25% for visibility
		} else if (gauge["bar"]) {
			// Fallback: Use the generic bar texture for EXP if tempExp doesn't exist
			Texture barTexture(gauge["bar"]);
			expbar = Gauge(Gauge::Type::V87_FILL, barTexture, exp_max, 0.0f);
		} else {
			// Final fallback: Create empty gauge if no textures exist
			expbar = Gauge();
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

		// V92: Different layout with HP/MP on the left, full-width status bar
		hpmp_pos = Point<int16_t>(110, 49);     // HP bar position 
		mp_pos = Point<int16_t>(215, 49);       // MP bar position 
		hpset_pos = Point<int16_t>(110, 35);   // HP value position (above HP bar)
		mpset_pos = Point<int16_t>(215, 35);   // MP value position (above MP bar)
		statset_pos = Point<int16_t>(450, 35); // EXP text position (right side)
		levelset_pos = Point<int16_t>(45, 50);  // Level position (left side)
		namelabel_pos = Point<int16_t>(85, 35);  // Character name position (top left)
		quickslot_pos = Point<int16_t>(VWIDTH - 200, 5); // Quickslot area position (right side)

		// Menu
		menu_pos = Point<int16_t>(682, -280);
		setting_pos = menu_pos + Point<int16_t>(0, 168);
		community_pos = menu_pos + Point<int16_t>(-26, 196);
		character_pos = menu_pos + Point<int16_t>(-61, 168);
		event_pos = menu_pos + Point<int16_t>(-94, 252);

		// V92: Positioning adjustments for different screen widths
		if (VWIDTH == 1280) {
			statset_pos = Point<int16_t>(580, 50);
			quickslot_pos = Point<int16_t>(VWIDTH - 200, 5);
			menu_pos += Point<int16_t>(-7, 0);
			setting_pos += Point<int16_t>(-7, 0);
			community_pos += Point<int16_t>(-7, 0);
			character_pos += Point<int16_t>(-7, 0);
			event_pos += Point<int16_t>(-7, 0);
		} else if (VWIDTH == 1366) {
			quickslot_pos = Point<int16_t>(VWIDTH - 200, 5);
			menu_pos += Point<int16_t>(-5, 0);
			setting_pos += Point<int16_t>(-5, 0);
			community_pos += Point<int16_t>(-5, 0);
			character_pos += Point<int16_t>(-5, 0);
			event_pos += Point<int16_t>(-5, 0);
		} else if (VWIDTH == 1920) {
			quickslot_pos = Point<int16_t>(VWIDTH - 200, 5);
			menu_pos += Point<int16_t>(272, 0);
			setting_pos += Point<int16_t>(272, 0);
			community_pos += Point<int16_t>(272, 0);
			character_pos += Point<int16_t>(272, 0);
			event_pos += Point<int16_t>(272, 0);
		}

		// V92: No separate HP/MP background sprites needed
		// The base background is already drawn, gauges render directly over it
		// Leave hpmp_sprites empty for v92

		// V92: Gauge width for v92 should be based on the actual texture size, not stretching
		// Use a more reasonable width for v92 gauges - they should not stretch across the screen
		int16_t hpmp_max = 120; // V92: Use a consistent, reasonable width that matches texture design

		// Create HP/MP gauges with v92 compatibility
		// V92: Use correct dedicated gauge textures as identified by researcher
			
		// Verify textures exist before creating gauges
		if (gauge["hpFlash"] && gauge["hpFlash"]["0"]) {
			// Use frame 0 of hpFlash/mpFlash for static display (avoid blinking)
			Texture hpTexture(gauge["hpFlash"]["0"]);
			hpbar = Gauge(Gauge::Type::V87_FILL, hpTexture, hpmp_max, 0.75f); // Test with 75% for visibility
		} else if (gauge["bar"]) {
			// Fallback: Use the generic bar texture for HP if flash textures don't exist
			Texture barTexture(gauge["bar"]);
			hpbar = Gauge(Gauge::Type::V87_FILL, barTexture, hpmp_max, 0.0f);
		} else {
			// Final fallback: Create empty gauge if no textures exist
			hpbar = Gauge();
		}
			
		if (gauge["mpFlash"] && gauge["mpFlash"]["0"]) {
			Texture mpTexture(gauge["mpFlash"]["0"]);
			mpbar = Gauge(Gauge::Type::V87_FILL, mpTexture, hpmp_max, 0.5f);  // Test with 50% for visibility
		} else if (gauge["bar"]) {
			// Fallback: Use the generic bar texture for MP if flash textures don't exist
			Texture barTexture(gauge["bar"]);
			mpbar = Gauge(Gauge::Type::V87_FILL, barTexture, hpmp_max, 0.3f);
		} else {
			// Final fallback: Create empty gauge if no textures exist
			mpbar = Gauge();
		}

		// Create character sets with v92 compatibility
		// V92: Use StatusBar number sprites for proper display
		nl::node numbers = statusBar["number"];
			
		if (numbers) {
			// Create charsets for HP/MP/EXP display using StatusBar number sprites
			statset = Charset(numbers, Charset::Alignment::LEFT);
			hpmpset = Charset(numbers, Charset::Alignment::LEFT);
			levelset = Charset(numbers, Charset::Alignment::LEFT);
		} else {
			// Fallback to default charsets if no number textures exist
			statset = Charset();
			hpmpset = Charset();
			levelset = Charset();
		}

		namelabel = OutlinedText(Text::Font::A13M, Text::Alignment::LEFT, Color::Name::GALLERY, Color::Name::TUNA);

		// Set quickslot textures with v87 compatibility
		// V87: Use empty textures to avoid wrong quickslot textures
		quickslot[0] = Texture();
		quickslot[1] = Texture();

		Point<int16_t> buttonPos = Point<int16_t>(591 + pos_adj, 73);

		if (VWIDTH == 1024)
			buttonPos += Point<int16_t>(38, 0);
		else if (VWIDTH == 1280)
			buttonPos += Point<int16_t>(31, 0);
		else if (VWIDTH == 1366)
			buttonPos += Point<int16_t>(33, 0);
		else if (VWIDTH == 1920)
			buttonPos += Point<int16_t>(310, 0);

		// V92: Create buttons from available assets with proper spacing
		// Position buttons on the right side of status bar
		Point<int16_t> v92ButtonPos = Point<int16_t>(VWIDTH - 350, 20); // Start position for buttons
			
		// Menu button
		if (statusBar["BtMenu"]) {
			buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(statusBar["BtMenu"], v92ButtonPos);
		}
			
		// Shop button
		if (statusBar["BtShop"]) {
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(statusBar["BtShop"], v92ButtonPos + Point<int16_t>(60, 0));
		}
			
		// Claim button
		if (statusBar["BtClaim"]) {
			buttons[Buttons::BT_EVENT] = std::make_unique<MapleButton>(statusBar["BtClaim"], v92ButtonPos + Point<int16_t>(120, 0));
		}
			
		// Add hotkey buttons (on the left side)
		Point<int16_t> v92HotkeyPos = Point<int16_t>(300, 20); // Position for hotkey buttons
			
		// Equipment inventory key
		if (statusBar["EquipKey"]) {
			buttons[Buttons::BT_EQUIP] = std::make_unique<MapleButton>(statusBar["EquipKey"], v92HotkeyPos);
		}
			
		// Item inventory key
		if (statusBar["InvenKey"]) {
			buttons[Buttons::BT_ITEM] = std::make_unique<MapleButton>(statusBar["InvenKey"], v92HotkeyPos + Point<int16_t>(40, 0));
		}
			
		// Stats key
		if (statusBar["StatKey"]) {
			buttons[Buttons::BT_STAT] = std::make_unique<MapleButton>(statusBar["StatKey"], v92HotkeyPos + Point<int16_t>(80, 0));
		}
			
		// Skills key
		if (statusBar["SkillKey"]) {
			buttons[Buttons::BT_SKILL] = std::make_unique<MapleButton>(statusBar["SkillKey"], v92HotkeyPos + Point<int16_t>(120, 0));
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

		// V92: No quickslot buttons needed for v92 - removed quickslot button creation


#pragma region Menu
		// Set menu backgrounds with v92 compatibility

		// V92: Create empty textures for menu backgrounds
		menubackground[0] = Texture();
		menubackground[1] = Texture();
		menubackground[2] = Texture();

		// Create menu buttons based on version
			
		if (statusBar["BtShop"]) {
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(statusBar["BtShop"], Point<int16_t>(580, 3));
		}
		if (statusBar["BtMenu"]) {
			buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(statusBar["BtMenu"], Point<int16_t>(633, 3));
		}
		if (statusBar["BtNPT"]) {
			buttons[Buttons::BT_OPTIONS] = std::make_unique<MapleButton>(statusBar["BtNPT"], Point<int16_t>(687, 3));
		}
			
		if (statusBar["EquipKey"]) {
			buttons[Buttons::BT_CHARACTER_EQUIP] = std::make_unique<MapleButton>(statusBar["EquipKey"], Point<int16_t>(235, 3));
		}
		if (statusBar["InvenKey"]) {
			buttons[Buttons::BT_CHARACTER_ITEM] = std::make_unique<MapleButton>(statusBar["InvenKey"], Point<int16_t>(268, 3));
		}
		if (statusBar["StatKey"]) {
			buttons[Buttons::BT_CHARACTER_STAT] = std::make_unique<MapleButton>(statusBar["StatKey"], Point<int16_t>(301, 3));
		}
		if (statusBar["SkillKey"]) {
			buttons[Buttons::BT_CHARACTER_SKILL] = std::make_unique<MapleButton>(statusBar["SkillKey"], Point<int16_t>(334, 3));
		}
			
		// v92: Create empty textures for menu titles
		menutitle[0] = Texture();
		menutitle[1] = Texture();
		menutitle[2] = Texture();
		menutitle[3] = Texture();
		menutitle[4] = Texture();
#pragma endregion

		// V92: Status bar stretches across bottom of screen for all resolutions
		if (VWIDTH == 800) {
			position = Point<int16_t>(0, VHEIGHT - 75);
			position_x = 0;
			position_y = position.y();
			dimension = Point<int16_t>(VWIDTH, 75);
		} else if (VWIDTH == 1024) {
			position = Point<int16_t>(0, VHEIGHT - 75);
			position_x = 0;
			position_y = position.y();
			dimension = Point<int16_t>(VWIDTH, 75);
		} else if (VWIDTH == 1280) {
			position = Point<int16_t>(0, VHEIGHT - 80);
			position_x = 0;
			position_y = position.y();
			dimension = Point<int16_t>(VWIDTH, 80);
		} else if (VWIDTH == 1366) {
			position = Point<int16_t>(0, VHEIGHT - 80);
			position_x = 0;
			position_y = position.y();
			dimension = Point<int16_t>(VWIDTH, 80);
		} else if (VWIDTH == 1920) {
			position = Point<int16_t>(0, VHEIGHT - 80);
			position_x = 0;
			position_y = position.y();
			dimension = Point<int16_t>(VWIDTH, 80);
		}
	}

	void UIStatusBar::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Draw all main buttons (including v92 hotkeys)
		for (size_t i = 0; i <= Buttons::BT_SKILL; i++)
			if (buttons.find(i) != buttons.end() && buttons.at(i))
				buttons.at(i)->draw(position);

		// V92: No HP/MP background sprites needed - they're part of the main background

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

		// V92: No quickslot buttons to draw

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
			case Buttons::BT_EQUIP:
			{
				UI::get().emplace<UIEquipInventory>(
					Stage::get().get_player().get_inventory()
				);
				break;
			}
			case Buttons::BT_ITEM:
			{
				UI::get().emplace<UIItemInventory>(
					Stage::get().get_player().get_inventory()
				);
				break;
			}
			case Buttons::BT_STAT:
			{
				UI::get().emplace<UIStatsInfo>(
					Stage::get().get_player().get_stats()
				);
				break;
			}
			case Buttons::BT_SKILL:
			{
				UI::get().emplace<UISkillBook>(
					Stage::get().get_player().get_stats(),
					Stage::get().get_player().get_skills()
				);
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
		
		// V92: No quickslot buttons to manage

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

		// V92: Manage menu buttons if they exist
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

		// V92: Manage setting buttons if they exist
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

		// V92: Manage community buttons if they exist
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

		// V92: Manage character buttons if they exist
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

		// V92: Manage event buttons if they exist
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