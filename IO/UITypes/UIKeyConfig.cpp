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
#include "UIKeyConfig.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../UITypes/UILoginNotice.h"
#include "../UITypes/UINotice.h"

#include "../../Data/ItemData.h"
#include "../../Data/SkillData.h"

#include "../../Net/Packets/PlayerPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIKeyConfig::UIKeyConfig(const Inventory& in_inventory, const SkillBook& in_skillbook) : UIDragElement<PosKEYCONFIG>(), inventory(in_inventory), skillbook(in_skillbook), dirty(false)
	{
		keyboard = &UI::get().get_keyboard();
		staged_mappings = keyboard->get_maplekeys();

		// v92/v87 compatibility: Try multiple locations for KeyConfig assets
		nl::node KeyConfig;
		
		// Try UIWindow.img first (v92)
		KeyConfig = nl::nx::UI["UIWindow.img"]["KeyConfig"];
		
		if (!KeyConfig) {
			// Try StatusBar.img (v87)
			KeyConfig = nl::nx::UI["StatusBar.img"]["KeyConfig"];
		}
		
		if (!KeyConfig) {
			// Try StatusBar3.img (newer versions)
			nl::node statusBar3 = nl::nx::UI["StatusBar3.img"];
			if (statusBar3) {
				KeyConfig = statusBar3["KeyConfig"];
			}
		}
		
		if (!KeyConfig) {
			// v92: No KeyConfig assets found - create minimal UI
			dimension = Point<int16_t>(600, 400);
			dragarea = Point<int16_t>(600, 20);
			
			// Add close button if available
			nl::node BtClose3 = nl::nx::UI["Basic.img"]["BtClose3"];
			if (BtClose3) {
				buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(BtClose3, Point<int16_t>(580, 3));
			}
			return;
		}

		// Load icon and key nodes with validation
		nl::node icon_node = KeyConfig["icon"];
		nl::node key_node = KeyConfig["key"];
		
		if (icon_node) {
			icon = icon_node;
		}
		
		if (key_node) {
			key = key_node;
		}

		nl::node backgrnd = KeyConfig["backgrnd"];
		if (!backgrnd) {
			// No background found - create minimal UI
			dimension = Point<int16_t>(600, 400);
			dragarea = Point<int16_t>(600, 20);
			return;
		}
		
		Texture bg = backgrnd;
		Point<int16_t> bg_dimensions = bg.get_dimensions();
		Point<int16_t> bg_origin = bg.get_origin();

		// v92: Adjust background position to align with key icons
		// Move background 3px right and 3px up from previous position (325,250)
		sprites.emplace_back(backgrnd, Point<int16_t>(328, 247));
		// Skip backgrnd2 and backgrnd3
		// sprites.emplace_back(KeyConfig["backgrnd2"]);
		// sprites.emplace_back(KeyConfig["backgrnd3"]);

		nl::node BtClose3 = nl::nx::UI["Basic.img"]["BtClose3"];
		if (BtClose3) {
			buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(BtClose3, Point<int16_t>(bg_dimensions.x() - 18, 3));
		}
		
		// Load buttons with fallback for v92
		nl::node btn_cancel = KeyConfig["button:Cancel"];
		if (btn_cancel) buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(btn_cancel);
		
		nl::node btn_default = KeyConfig["button:Default"];
		if (btn_default) buttons[Buttons::DEFAULT] = std::make_unique<MapleButton>(btn_default);
		
		nl::node btn_delete = KeyConfig["button:Delete"];
		if (btn_delete) buttons[Buttons::DELETE] = std::make_unique<MapleButton>(btn_delete);
		
		nl::node btn_keysetting = KeyConfig["button:keySetting"];
		if (btn_keysetting) buttons[Buttons::KEYSETTING] = std::make_unique<MapleButton>(btn_keysetting);
		
		nl::node btn_ok = KeyConfig["button:OK"];
		if (btn_ok) buttons[Buttons::OK] = std::make_unique<MapleButton>(btn_ok);

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(bg_dimensions.x(), 20);

		load_keys_pos();
		load_unbound_actions_pos();
		load_key_textures();
		load_actions();
		load_icons();

		bind_staged_action_keys();
	}

	/// Load
	void UIKeyConfig::load_keys_pos()
	{
		int16_t slot_width = 33;
		int16_t slot_width_lg = 98;
		int16_t slot_height = 33;

		int16_t row_y = 126;
		int16_t row_special_y = row_y - slot_height - 5;

		int16_t row_quickslot_x = 535;

		int16_t row_one_x = 31;
		int16_t row_two_x = 80;
		int16_t row_three_x = 96;
		int16_t row_four_x = 55;
		int16_t row_five_x = 39;

		int16_t row_special_x = row_one_x;

		keys_pos[KeyConfig::Key::ESCAPE] = Point<int16_t>(row_one_x, row_special_y);

		row_special_x += slot_width * 2;

		for (size_t i = KeyConfig::Key::F1; i <= KeyConfig::Key::F12; i++)
		{
			KeyConfig::Key id = KeyConfig::actionbyid(i);

			keys_pos[id] = Point<int16_t>(row_special_x, row_special_y);

			row_special_x += slot_width;

			if (id == KeyConfig::Key::F4 || id == KeyConfig::Key::F8)
				row_special_x += 17;
		}

		keys_pos[KeyConfig::Key::SCROLL_LOCK] = Point<int16_t>(row_quickslot_x + (slot_width * 1), row_special_y);

		keys_pos[KeyConfig::Key::GRAVE_ACCENT] = Point<int16_t>(row_one_x + (slot_width * 0), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM1] = Point<int16_t>(row_one_x + (slot_width * 1), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM2] = Point<int16_t>(row_one_x + (slot_width * 2), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM3] = Point<int16_t>(row_one_x + (slot_width * 3), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM4] = Point<int16_t>(row_one_x + (slot_width * 4), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM5] = Point<int16_t>(row_one_x + (slot_width * 5), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM6] = Point<int16_t>(row_one_x + (slot_width * 6), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM7] = Point<int16_t>(row_one_x + (slot_width * 7), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM8] = Point<int16_t>(row_one_x + (slot_width * 8), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM9] = Point<int16_t>(row_one_x + (slot_width * 9), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::NUM0] = Point<int16_t>(row_one_x + (slot_width * 10), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::MINUS] = Point<int16_t>(row_one_x + (slot_width * 11), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::EQUAL] = Point<int16_t>(row_one_x + (slot_width * 12), row_y + (slot_height * 0));

		for (size_t i = KeyConfig::Key::Q; i <= KeyConfig::Key::RIGHT_BRACKET; i++)
		{
			KeyConfig::Key id = KeyConfig::actionbyid(i);

			keys_pos[id] = Point<int16_t>(row_two_x + (slot_width * (i - KeyConfig::Key::Q)), row_y + (slot_height * 1));
		}

		row_two_x += 9;

		keys_pos[KeyConfig::Key::BACKSLASH] = Point<int16_t>(row_two_x + (slot_width * 12), row_y + (slot_height * 1));

		for (size_t i = KeyConfig::Key::A; i <= KeyConfig::Key::APOSTROPHE; i++)
		{
			KeyConfig::Key id = KeyConfig::actionbyid(i);

			keys_pos[id] = Point<int16_t>(row_three_x + (slot_width * (i - KeyConfig::Key::A)), row_y + (slot_height * 2));
		}

		keys_pos[KeyConfig::Key::LEFT_SHIFT] = Point<int16_t>(row_four_x + (slot_width * 0), row_y + (slot_height * 3));

		row_four_x += 24;

		for (size_t i = KeyConfig::Key::Z; i <= KeyConfig::Key::PERIOD; i++)
		{
			KeyConfig::Key id = KeyConfig::actionbyid(i);

			keys_pos[id] = Point<int16_t>(row_four_x + (slot_width * (i - KeyConfig::Key::Z + 1)), row_y + (slot_height * 3));
		}

		row_four_x += 24;

		keys_pos[KeyConfig::Key::RIGHT_SHIFT] = Point<int16_t>(row_four_x + (slot_width * 11), row_y + (slot_height * 3));

		keys_pos[KeyConfig::Key::LEFT_CONTROL] = Point<int16_t>(row_five_x + (slot_width_lg * 0), row_y + (slot_height * 4));
		keys_pos[KeyConfig::Key::LEFT_ALT] = Point<int16_t>(row_five_x + (slot_width_lg * 1), row_y + (slot_height * 4));

		row_five_x += 24;

		keys_pos[KeyConfig::Key::SPACE] = Point<int16_t>(row_five_x + (slot_width_lg * 2), row_y + (slot_height * 4));

		row_five_x += 27;

		keys_pos[KeyConfig::Key::RIGHT_ALT] = Point<int16_t>(row_five_x + (slot_width_lg * 3), row_y + (slot_height * 4));

		row_five_x += 2;

		keys_pos[KeyConfig::Key::RIGHT_CONTROL] = Point<int16_t>(row_five_x + (slot_width_lg * 4), row_y + (slot_height * 4));

		keys_pos[KeyConfig::Key::INSERT] = Point<int16_t>(row_quickslot_x + (slot_width * 0), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::HOME] = Point<int16_t>(row_quickslot_x + (slot_width * 1), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::PAGE_UP] = Point<int16_t>(row_quickslot_x + (slot_width * 2), row_y + (slot_height * 0));
		keys_pos[KeyConfig::Key::DELETE] = Point<int16_t>(row_quickslot_x + (slot_width * 0), row_y + (slot_height * 1));
		keys_pos[KeyConfig::Key::END] = Point<int16_t>(row_quickslot_x + (slot_width * 1), row_y + (slot_height * 1));
		keys_pos[KeyConfig::Key::PAGE_DOWN] = Point<int16_t>(row_quickslot_x + (slot_width * 2), row_y + (slot_height * 1));
	}

	void UIKeyConfig::load_unbound_actions_pos()
	{
		int16_t row_x = 26;
		int16_t row_y = 307;

		int16_t slot_width = 36;
		int16_t slot_height = 36;

		/// Row 1
		unbound_actions_pos[KeyAction::Id::MAPLECHAT] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::TOGGLECHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::WHISPER] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::MEDALS] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::BOSSPARTY] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::PROFESSION] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::EQUIPMENT] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::ITEMS] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::CHARINFO] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::MENU] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::QUICKSLOTS] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::PICKUP] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::SIT] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::ATTACK] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::JUMP] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::INTERACT_HARVEST] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 0));
		unbound_actions_pos[KeyAction::Id::SOULWEAPON] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 0));

		/// Row 2
		unbound_actions_pos[KeyAction::Id::SAY] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::PARTYCHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::FRIENDSCHAT] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::ITEMPOT] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::EVENT] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::SILENTCRUSADE] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::STATS] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::SKILLS] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::QUESTLOG] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::CHANGECHANNEL] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::GUILD] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::PARTY] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::NOTIFIER] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::FRIENDS] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::WORLDMAP] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::MINIMAP] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 1));
		unbound_actions_pos[KeyAction::Id::KEYBINDINGS] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 1));

		/// Row 3
		unbound_actions_pos[KeyAction::Id::GUILDCHAT] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::ALLIANCECHAT] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::BATTLEANALYSIS] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::GUIDE] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::ENHANCEEQUIP] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MONSTERCOLLECTION] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MANAGELEGION] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 2));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MAPLENEWS] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::CASHSHOP] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MAINMENU] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::SCREENSHOT] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::PICTUREMODE] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 2));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MUTE] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 2));
		unbound_actions_pos[KeyAction::Id::MAPLERELAY] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 2));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 2));

		/// Row 4
		unbound_actions_pos[KeyAction::Id::FACE1] = Point<int16_t>(row_x + (slot_width * 0), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE2] = Point<int16_t>(row_x + (slot_width * 1), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE3] = Point<int16_t>(row_x + (slot_width * 2), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE4] = Point<int16_t>(row_x + (slot_width * 3), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE5] = Point<int16_t>(row_x + (slot_width * 4), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE6] = Point<int16_t>(row_x + (slot_width * 5), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FACE7] = Point<int16_t>(row_x + (slot_width * 6), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::MAPLEACHIEVEMENT] = Point<int16_t>(row_x + (slot_width * 7), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::FAMILIAR] = Point<int16_t>(row_x + (slot_width * 8), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::TOSPOUSE] = Point<int16_t>(row_x + (slot_width * 9), row_y + (slot_height * 3));
		unbound_actions_pos[KeyAction::Id::EMOTICON] = Point<int16_t>(row_x + (slot_width * 10), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 11), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 12), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 13), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 14), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 15), row_y + (slot_height * 3));
		//unbound_actions_pos[KeyAction::Id::LENGTH] = Point<int16_t>(row_x + (slot_width * 16), row_y + (slot_height * 3));
	}

	void UIKeyConfig::load_key_textures()
	{
		// Check if key node exists before loading textures
		if (!key) {
			return;
		}
		
		// Helper lambda to safely load key texture
		auto load_key = [&](KeyConfig::Key k, int index) {
			if (key[index]) {
				key_textures[k] = key[index];
			}
		};
		
		// Load key textures with bounds checking
		load_key(KeyConfig::Key::ESCAPE, 1);
		load_key(KeyConfig::Key::NUM1, 2);
		load_key(KeyConfig::Key::NUM2, 3);
		load_key(KeyConfig::Key::NUM3, 4);
		load_key(KeyConfig::Key::NUM4, 5);
		load_key(KeyConfig::Key::NUM5, 6);
		load_key(KeyConfig::Key::NUM6, 7);
		load_key(KeyConfig::Key::NUM7, 8);
		load_key(KeyConfig::Key::NUM8, 9);
		load_key(KeyConfig::Key::NUM9, 10);
		load_key(KeyConfig::Key::NUM0, 11);
		load_key(KeyConfig::Key::MINUS, 12);
		load_key(KeyConfig::Key::EQUAL, 13);

		// Continue loading remaining keys
		load_key(KeyConfig::Key::Q, 16);
		load_key(KeyConfig::Key::W, 17);
		load_key(KeyConfig::Key::E, 18);
		load_key(KeyConfig::Key::R, 19);
		load_key(KeyConfig::Key::T, 20);
		load_key(KeyConfig::Key::Y, 21);
		load_key(KeyConfig::Key::U, 22);
		load_key(KeyConfig::Key::I, 23);
		load_key(KeyConfig::Key::O, 24);
		load_key(KeyConfig::Key::P, 25);
		load_key(KeyConfig::Key::LEFT_BRACKET, 26);
		load_key(KeyConfig::Key::RIGHT_BRACKET, 27);

		load_key(KeyConfig::Key::LEFT_CONTROL, 29);
		load_key(KeyConfig::Key::RIGHT_CONTROL, 29);

		load_key(KeyConfig::Key::A, 30);
		load_key(KeyConfig::Key::S, 31);
		load_key(KeyConfig::Key::D, 32);
		load_key(KeyConfig::Key::F, 33);
		load_key(KeyConfig::Key::G, 34);
		load_key(KeyConfig::Key::H, 35);
		load_key(KeyConfig::Key::J, 36);
		load_key(KeyConfig::Key::K, 37);
		load_key(KeyConfig::Key::L, 38);
		load_key(KeyConfig::Key::SEMICOLON, 39);
		load_key(KeyConfig::Key::APOSTROPHE, 40);
		load_key(KeyConfig::Key::GRAVE_ACCENT, 41);

		load_key(KeyConfig::Key::LEFT_SHIFT, 42);
		load_key(KeyConfig::Key::RIGHT_SHIFT, 42);

		load_key(KeyConfig::Key::BACKSLASH, 43);
		load_key(KeyConfig::Key::Z, 44);
		load_key(KeyConfig::Key::X, 45);
		load_key(KeyConfig::Key::C, 46);
		load_key(KeyConfig::Key::V, 47);
		load_key(KeyConfig::Key::B, 48);
		load_key(KeyConfig::Key::N, 49);
		load_key(KeyConfig::Key::M, 50);
		load_key(KeyConfig::Key::COMMA, 51);
		load_key(KeyConfig::Key::PERIOD, 52);

		load_key(KeyConfig::Key::LEFT_ALT, 56);
		load_key(KeyConfig::Key::RIGHT_ALT, 56);

		load_key(KeyConfig::Key::SPACE, 57);

		load_key(KeyConfig::Key::F1, 59);
		load_key(KeyConfig::Key::F2, 60);
		load_key(KeyConfig::Key::F3, 61);
		load_key(KeyConfig::Key::F4, 62);
		load_key(KeyConfig::Key::F5, 63);
		load_key(KeyConfig::Key::F6, 64);
		load_key(KeyConfig::Key::F7, 65);
		load_key(KeyConfig::Key::F8, 66);
		load_key(KeyConfig::Key::F9, 67);
		load_key(KeyConfig::Key::F10, 68);

		load_key(KeyConfig::Key::SCROLL_LOCK, 70);
		load_key(KeyConfig::Key::HOME, 71);

		load_key(KeyConfig::Key::PAGE_UP, 73);

		load_key(KeyConfig::Key::END, 79);

		load_key(KeyConfig::Key::PAGE_DOWN, 81);
		load_key(KeyConfig::Key::INSERT, 82);
		load_key(KeyConfig::Key::DELETE, 83);

		load_key(KeyConfig::Key::F11, 87);
		load_key(KeyConfig::Key::F12, 88);
	}

	void UIKeyConfig::load_actions()
	{
		for (size_t i = KeyAction::Id::EQUIPMENT; i < KeyAction::Id::LENGTH; i++)
		{
			if (i == KeyAction::Id::SAFEMODE || i == KeyAction::Id::MAPLESTORAGE || i == KeyAction::Id::VIEWERSCHAT || i == KeyAction::Id::BITS)
				continue;

			if (icon[i])
			{
				KeyAction::Id action = KeyAction::actionbyid(i);

				action_mappings.push_back(Keyboard::Mapping(get_keytype(action), action));

				action_icons[action] = std::make_unique<Icon>(
					std::make_unique<KeyIcon>(action),
					icon[i],
					-1
					);
			}
		}
	}

	void UIKeyConfig::load_icons()
	{
		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping mapping = it.second;
			int32_t id = mapping.action;

			// Fix: Skip item ID 0 to prevent freezes
			if (mapping.type == KeyType::Id::ITEM && id > 0)
			{
				int16_t count = inventory.get_total_item_count(id);

				item_icons[id] = std::make_unique<Icon>(
					std::make_unique<KeyIcon>(mapping, count),
					get_item_texture(id),
					count
					);
			}
			else if (mapping.type == KeyType::Id::SKILL && id > 0)
			{
				// Verify skill texture exists before creating icon
				Texture skill_texture = get_skill_texture(id);
				if (skill_texture.is_valid()) {
					int16_t count = -1;

					skill_icons[id] = std::make_unique<Icon>(
						std::make_unique<KeyIcon>(mapping, count),
						skill_texture,
						count
						);
				}
			}
		}
	}

	/// UI: General
	void UIKeyConfig::draw(float inter) const
	{
		UIElement::draw(inter);

		// Bound Keys
		for (auto const& iter : staged_mappings)
		{
			Keyboard::Mapping mapping = iter.second;

			if (mapping.type != KeyType::Id::NONE)
			{
				int32_t id = mapping.action;
				Icon* icon = nullptr;

				if (mapping.type == KeyType::Id::ITEM)
				{
					icon = item_icons.at(id).get();
				}
				else if (mapping.type == KeyType::Id::SKILL)
				{
					// Safety check: only access skill icon if it exists
					auto skill_iter = skill_icons.find(id);
					if (skill_iter != skill_icons.end()) {
						icon = skill_iter->second.get();
					}
				}
				else if (is_action_mapping(mapping))
				{
					KeyAction::Id action = KeyAction::actionbyid(mapping.action);

					if (action < KeyAction::Id::LENGTH)
						icon = action_icons[action].get();
				}
				else
				{
					LOG(LOG_DEBUG, "Invalid mapping (" << mapping.type << ", " << mapping.action << ") for key [" << iter.first << "].");
				}

				if (icon)
				{
					KeyConfig::Key key = KeyConfig::actionbyid(iter.first);

					if (key < KeyConfig::Key::LENGTH)
					{
						if (key == KeyConfig::Key::SPACE)
						{
							icon->draw(position + keys_pos[key] - Point<int16_t>(0, 3));
						}
						else if (key == KeyConfig::Key::LEFT_CONTROL || key == KeyConfig::Key::RIGHT_CONTROL)
						{
							icon->draw(position + keys_pos[KeyConfig::Key::LEFT_CONTROL] - Point<int16_t>(2, 3));
							icon->draw(position + keys_pos[KeyConfig::Key::RIGHT_CONTROL] - Point<int16_t>(2, 3));
						}
						else if (key == KeyConfig::Key::LEFT_ALT || key == KeyConfig::Key::RIGHT_ALT)
						{
							icon->draw(position + keys_pos[KeyConfig::Key::LEFT_ALT] - Point<int16_t>(2, 3));
							icon->draw(position + keys_pos[KeyConfig::Key::RIGHT_ALT] - Point<int16_t>(2, 3));
						}
						else if (key == KeyConfig::Key::LEFT_SHIFT || key == KeyConfig::Key::RIGHT_SHIFT)
						{
							icon->draw(position + keys_pos[KeyConfig::Key::LEFT_SHIFT] - Point<int16_t>(2, 3));
							icon->draw(position + keys_pos[KeyConfig::Key::RIGHT_SHIFT] - Point<int16_t>(2, 3));
						}
						else
						{
							icon->draw(position + keys_pos[key] - Point<int16_t>(2, 3));
						}
					}
				}
			}
		}

		// Unbound Keys
		for (const auto& icon : action_icons)
			if (icon.second)
				if (std::find(bound_actions.begin(), bound_actions.end(), icon.first) == bound_actions.end())
					icon.second->draw(position + unbound_actions_pos[icon.first]);

		// Keys
		for (const auto& key_texture : key_textures)
		{
			KeyConfig::Key key = key_texture.first;
			Texture texture = key_texture.second;

			texture.draw(position + keys_pos[key]);
		}
	}

	void UIKeyConfig::close()
	{
		clear_tooltip();
		deactivate();
		reset();
	}

	Button::State UIKeyConfig::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::CLOSE:
			case Buttons::CANCEL:
			{
				safe_close();

				return Button::State::NORMAL;
			}
			case Buttons::DEFAULT:
			{
				static const std::string& message = "Would you like to revert to default settings?";

				auto onok = [&](bool ok)
				{
					if (ok)
					{
						auto keysel_onok = [&](bool alternate)
						{
							clear();

							if (alternate)
								staged_mappings = alternate_keys;
							else
								staged_mappings = basic_keys;

							bind_staged_action_keys();
						};

						UI::get().emplace<UIKeySelect>(keysel_onok, false);
					}
				};

				UI::get().emplace<UIOk>(message, onok);

				return Button::State::NORMAL;
			}
			case Buttons::DELETE:
			{
				static const std::string& message = "Would you like to clear all key bindings?";

				auto onok = [&](bool ok)
				{
					if (ok)
						clear();
				};

				UI::get().emplace<UIOk>(message, onok);

				return Button::State::NORMAL;
			}
			case Buttons::OK:
			{
				save_staged_mappings();
				close();

				return Button::State::NORMAL;
			}
			default:
			{
				return Button::State::PRESSED;
			}
		}
	}

	Cursor::State UIKeyConfig::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		if (clicked)
		{
			KeyAction::Id action = unbound_action_by_position(cursorpos);

			if (action < KeyAction::Id::LENGTH)
			{
				if (auto icon = action_icons[action].get())
				{
					icon->start_drag(cursorpos - position - unbound_actions_pos[action]);

					UI::get().drag_icon(icon);

					return Cursor::State::GRABBING;
				}
			}

			KeyConfig::Key key = key_by_position(cursorpos);

			if (key < KeyConfig::Key::LENGTH)
			{
				Keyboard::Mapping mapping = get_staged_mapping(key);

				if (mapping.type != KeyType::Id::NONE)
				{
					int32_t id = mapping.action;
					Icon* icon = nullptr;

					if (mapping.type == KeyType::Id::ITEM)
					{
						icon = item_icons[id].get();
					}
					else if (mapping.type == KeyType::Id::SKILL)
					{
						// Safety check: only access skill icon if it exists
						auto skill_iter = skill_icons.find(id);
						if (skill_iter != skill_icons.end()) {
							icon = skill_iter->second.get();
						}
					}
					else if (is_action_mapping(mapping))
					{
						KeyAction::Id action = KeyAction::actionbyid(mapping.action);

						if (action < KeyAction::Id::LENGTH)
							icon = action_icons[action].get();
					}
					else
					{
						LOG(LOG_DEBUG, "Invalid mapping (" << mapping.type << ", " << mapping.action << ") for key [" << key << "].");
					}

					if (icon)
					{
						clear_tooltip();

						icon->start_drag(cursorpos - position - keys_pos[key]);

						UI::get().drag_icon(icon);

						return Cursor::State::GRABBING;
					}
				}
			}
		}
		else
		{
			KeyConfig::Key key = key_by_position(cursorpos);

			if (key < KeyConfig::Key::LENGTH)
			{
				Keyboard::Mapping mapping = get_staged_mapping(key);

				if (mapping.type == KeyType::Id::ITEM || mapping.type == KeyType::Id::SKILL)
				{
					int32_t id = mapping.action;

					if (mapping.type == KeyType::Id::ITEM)
						show_item(id);
					else
						show_skill(id);
				}
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	bool UIKeyConfig::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		for (auto iter : unbound_actions_pos)
		{
			Rectangle<int16_t> icon_rect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(32, 32)
				);

			if (icon_rect.contains(cursorpos))
				icon.drop_on_bindings(cursorpos, true);
		}

		KeyConfig::Key fkey = key_by_position(cursorpos);

		if (fkey != KeyConfig::Key::LENGTH)
			icon.drop_on_bindings(cursorpos, false);

		return true;
	}

	void UIKeyConfig::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
			safe_close();
	}

	UIElement::Type UIKeyConfig::get_type() const
	{
		return TYPE;
	}

	void UIKeyConfig::safe_close()
	{
		if (dirty)
		{
			// Skip dialog due to text rendering overflow issue - just save and close
			save_staged_mappings();
			close();
		}
		else
		{
			close();
		}
	}

	/// UI: Tooltip
	void UIKeyConfig::show_item(int32_t item_id)
	{
		UI::get().show_item(Tooltip::Parent::KEYCONFIG, item_id);
	}

	void UIKeyConfig::show_skill(int32_t skill_id)
	{
		int32_t level = skillbook.get_level(skill_id);
		int32_t masterlevel = skillbook.get_masterlevel(skill_id);
		int64_t expiration = skillbook.get_expiration(skill_id);

		UI::get().show_skill(Tooltip::Parent::KEYCONFIG, skill_id, level, masterlevel, expiration);
	}

	void UIKeyConfig::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::KEYCONFIG);
	}

	/// Keymap Staging
	void UIKeyConfig::stage_mapping(Point<int16_t> cursorposition, Keyboard::Mapping mapping)
	{
		KeyConfig::Key key = key_by_position(cursorposition);
		Keyboard::Mapping prior_staged = staged_mappings[key];

		if (prior_staged == mapping)
			return;

		unstage_mapping(prior_staged);

		int32_t id = mapping.action;

		if (is_action_mapping(mapping))
		{
			KeyAction::Id action = KeyAction::actionbyid(id);

			if (std::find(bound_actions.begin(), bound_actions.end(), action) == bound_actions.end())
				bound_actions.emplace_back(action);
		}

		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping staged_mapping = it.second;

			if (staged_mapping == mapping)
			{
				if (it.first == KeyConfig::Key::LEFT_CONTROL || it.first == KeyConfig::Key::RIGHT_CONTROL)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_CONTROL);
					staged_mappings.erase(KeyConfig::Key::RIGHT_CONTROL);
				}
				else if (it.first == KeyConfig::Key::LEFT_ALT || it.first == KeyConfig::Key::RIGHT_ALT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_ALT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_ALT);
				}
				else if (it.first == KeyConfig::Key::LEFT_SHIFT || it.first == KeyConfig::Key::RIGHT_SHIFT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_SHIFT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_SHIFT);
				}
				else
				{
					staged_mappings.erase(it.first);
				}

				break;
			}
		}

		if (key == KeyConfig::Key::LEFT_CONTROL || key == KeyConfig::Key::RIGHT_CONTROL)
		{
			staged_mappings[KeyConfig::Key::LEFT_CONTROL] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_CONTROL] = mapping;
		}
		else if (key == KeyConfig::Key::LEFT_ALT || key == KeyConfig::Key::RIGHT_ALT)
		{
			staged_mappings[KeyConfig::Key::LEFT_ALT] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_ALT] = mapping;
		}
		else if (key == KeyConfig::Key::LEFT_SHIFT || key == KeyConfig::Key::RIGHT_SHIFT)
		{
			staged_mappings[KeyConfig::Key::LEFT_SHIFT] = mapping;
			staged_mappings[KeyConfig::Key::RIGHT_SHIFT] = mapping;
		}
		else
		{
			staged_mappings[key] = mapping;
		}

		if (mapping.type == KeyType::Id::ITEM)
		{
			if (item_icons.find(id) == item_icons.end())
			{
				int16_t count = inventory.get_total_item_count(id);

				item_icons[id] = std::make_unique<Icon>(
					std::make_unique<KeyIcon>(mapping, count),
					get_item_texture(id),
					count
					);
			}
		}
		else if (mapping.type == KeyType::Id::SKILL)
		{
			if (skill_icons.find(id) == skill_icons.end())
			{
				int16_t count = -1;

				skill_icons[id] = std::make_unique<Icon>(
					std::make_unique<KeyIcon>(mapping, count),
					get_skill_texture(id),
					count
					);
			}
		}

		dirty = true;
	}

	void UIKeyConfig::unstage_mapping(Keyboard::Mapping mapping)
	{
		if (is_action_mapping(mapping))
		{
			KeyAction::Id action = KeyAction::actionbyid(mapping.action);
			auto iter = std::find(bound_actions.begin(), bound_actions.end(), action);

			if (iter != bound_actions.end())
				bound_actions.erase(iter);
		}

		for (auto const& it : staged_mappings)
		{
			Keyboard::Mapping staged_mapping = it.second;

			if (staged_mapping == mapping)
			{
				if (it.first == KeyConfig::Key::LEFT_CONTROL || it.first == KeyConfig::Key::RIGHT_CONTROL)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_CONTROL);
					staged_mappings.erase(KeyConfig::Key::RIGHT_CONTROL);
				}
				else if (it.first == KeyConfig::Key::LEFT_ALT || it.first == KeyConfig::Key::RIGHT_ALT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_ALT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_ALT);
				}
				else if (it.first == KeyConfig::Key::LEFT_SHIFT || it.first == KeyConfig::Key::RIGHT_SHIFT)
				{
					staged_mappings.erase(KeyConfig::Key::LEFT_SHIFT);
					staged_mappings.erase(KeyConfig::Key::RIGHT_SHIFT);
				}
				else
				{
					staged_mappings.erase(it.first);
				}

				if (staged_mapping.type == KeyType::Id::ITEM)
				{
					int32_t item_id = staged_mapping.action;
					item_icons.erase(item_id);
				}
				else if (staged_mapping.type == KeyType::Id::SKILL)
				{
					int32_t skill_id = staged_mapping.action;
					skill_icons.erase(skill_id);
				}

				dirty = true;

				break;
			}
		}
	}

	void UIKeyConfig::save_staged_mappings()
	{
		std::vector<std::tuple<KeyConfig::Key, KeyType::Id, int32_t>> updated_actions;

		for (auto& key : staged_mappings)
		{
			KeyConfig::Key k = KeyConfig::actionbyid(key.first);
			Keyboard::Mapping mapping = key.second;
			Keyboard::Mapping saved_mapping = keyboard->get_maple_mapping(key.first);

			if (mapping != saved_mapping)
				updated_actions.emplace_back(std::make_tuple(k, mapping.type, mapping.action));
		}

		auto maplekeys = keyboard->get_maplekeys();

		for (auto& key : maplekeys)
		{
			bool keyFound = false;
			KeyConfig::Key keyConfig = KeyConfig::actionbyid(key.first);

			for (auto& tkey : staged_mappings)
			{
				KeyConfig::Key tKeyConfig = KeyConfig::actionbyid(tkey.first);

				if (keyConfig == tKeyConfig)
				{
					keyFound = true;
					break;
				}
			}

			if (!keyFound)
				updated_actions.emplace_back(std::make_tuple(keyConfig, KeyType::Id::NONE, KeyAction::Id::LENGTH));
		}

		if (updated_actions.size() > 0)
			ChangeKeyMapPacket(updated_actions).dispatch();

		for (auto& action : updated_actions)
		{
			KeyConfig::Key key = std::get<0>(action);
			KeyType::Id type = std::get<1>(action);
			int32_t keyAction = std::get<2>(action);

			if (type == KeyType::Id::NONE)
				keyboard->remove(key);
			else
				keyboard->assign(key, type, keyAction);
		}

		dirty = false;
	}

	void UIKeyConfig::bind_staged_action_keys()
	{
		for (auto fkey : key_textures)
		{
			Keyboard::Mapping mapping = get_staged_mapping(fkey.first);

			if (mapping.type != KeyType::Id::NONE)
			{
				KeyAction::Id action = KeyAction::actionbyid(mapping.action);

				if (action < KeyAction::Id::LENGTH)
				{
					KeyType::Id type = get_keytype(action);

					if (type == KeyType::Id::NONE || mapping.type != type)
						continue;

					bound_actions.emplace_back(action);
				}
			}
		}
	}

	void UIKeyConfig::clear()
	{
		item_icons.clear();
		skill_icons.clear();
		bound_actions.clear();
		staged_mappings = {};
		dirty = true;
	}

	void UIKeyConfig::reset()
	{
		clear();

		staged_mappings = keyboard->get_maplekeys();

		load_icons();
		bind_staged_action_keys();

		dirty = false;
	}

	/// Helpers
	Texture UIKeyConfig::get_item_texture(int32_t item_id) const
	{
		const ItemData& data = ItemData::get(item_id);
		return data.get_icon(false);
	}

	Texture UIKeyConfig::get_skill_texture(int32_t skill_id) const
	{
		const SkillData& data = SkillData::get(skill_id);
		return data.get_icon(SkillData::Icon::NORMAL);
	}

	KeyConfig::Key UIKeyConfig::key_by_position(Point<int16_t> cursorpos) const
	{
		for (auto iter : keys_pos)
		{
			Rectangle<int16_t> icon_rect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(32, 32)
				);

			if (icon_rect.contains(cursorpos))
				return iter.first;
		}

		return KeyConfig::Key::LENGTH;
	}

	KeyAction::Id UIKeyConfig::unbound_action_by_position(Point<int16_t> cursorpos) const
	{
		for (auto iter : unbound_actions_pos)
		{
			if (std::find(bound_actions.begin(), bound_actions.end(), iter.first) != bound_actions.end())
				continue;

			Rectangle<int16_t> icon_rect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(32, 32)
				);

			if (icon_rect.contains(cursorpos))
				return iter.first;
		}

		return KeyAction::Id::LENGTH;
	}

	Keyboard::Mapping UIKeyConfig::get_staged_mapping(int32_t keycode) const
	{
		auto iter = staged_mappings.find(keycode);

		if (iter == staged_mappings.end())
			return {};

		return iter->second;
	}

	bool UIKeyConfig::is_action_mapping(Keyboard::Mapping mapping) const
	{
		return std::find(action_mappings.begin(), action_mappings.end(), mapping) != action_mappings.end();
	}

	KeyType::Id UIKeyConfig::get_keytype(KeyAction::Id action)
	{
		if (action == KeyAction::Id::PICKUP || action == KeyAction::Id::SIT || action == KeyAction::Id::ATTACK || action == KeyAction::Id::JUMP || action == KeyAction::Id::INTERACT_HARVEST)
			return KeyType::Id::ACTION;
		else if (action < KeyAction::Id::FACE1 || action > KeyAction::Id::FACE7 && action < KeyAction::Id::LEFT)
			return KeyType::Id::MENU;
		else if (action >= KeyAction::Id::FACE1 && action <= KeyAction::Id::FACE7)
			return KeyType::Id::FACE;
		else
			return KeyType::Id::NONE;
	}

	/// Item count
	void UIKeyConfig::update_item_count(InventoryType::Id type, int16_t slot, int16_t change)
	{
		int32_t item_id = inventory.get_item_id(type, slot);

		if (item_icons.find(item_id) == item_icons.end())
			return;

		int16_t item_count = item_icons[item_id]->get_count();
		item_icons[item_id]->set_count(item_count + change);
	}

	/// MappingIcon
	UIKeyConfig::KeyIcon::KeyIcon(Keyboard::Mapping mapping, int16_t count) : mapping(mapping), count(count) {}

	UIKeyConfig::KeyIcon::KeyIcon(KeyAction::Id action)
	{
		KeyType::Id type = UIKeyConfig::get_keytype(action);
		mapping = Keyboard::Mapping(type, action);
	}

	void UIKeyConfig::KeyIcon::drop_on_stage() const
	{
		if (mapping.type == KeyType::Id::ITEM || mapping.type == KeyType::Id::SKILL)
		{
			auto keyconfig = UI::get().get_element<UIKeyConfig>();
			keyconfig->unstage_mapping(mapping);
		}
	}

	void UIKeyConfig::KeyIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		auto keyconfig = UI::get().get_element<UIKeyConfig>();

		if (remove)
			keyconfig->unstage_mapping(mapping);
		else
			keyconfig->stage_mapping(cursorposition, mapping);
	}

	void UIKeyConfig::KeyIcon::set_count(int16_t c)
	{
		count = c;
	}

	Icon::IconType UIKeyConfig::KeyIcon::get_type()
	{
		return Icon::IconType::KEY;
	}
}