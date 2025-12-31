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
#include "../../Net/Packets/PlayerPackets.h"
#include "../../Data/ItemData.h"
#include "../../Data/SkillData.h"
#include "../../Character/Inventory/Inventory.h"
#include "../Keyboard.h"
#include "../KeyConfig.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIStatusBar::UIStatusBar(const CharStats& st) : stats(st)
	{
		quickslot_active = true;
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
		base_width = 0;
		if (background) {
			Texture base_tex(background);
			base_width = base_tex.width();
			sprites.emplace_back(background, DrawArgument(Point<int16_t>(0, 0)));
		}
		
		// Add backgrnd2 - secondary background element
		nl::node background2 = base["backgrnd2"];
		if (background2) {
			sprites.emplace_back(background2, DrawArgument(Point<int16_t>(1, 0)));
		}

		// Add gauge.bar - the bar texture that goes under graduation
		nl::node bar = gauge["bar"];
		if (bar) {
			sprites.emplace_back(bar, Point<int16_t>(217, 38)); // +1 y
		}
		// Add gauge.graduation - shows the empty bar outlines for HP/MP/XP
		nl::node graduation = gauge["graduation"];
		if (graduation) {
			sprites.emplace_back(graduation, DrawArgument(Point<int16_t>(217, 37))); // -1 y
		}
		
		// Add base.chat - chat interface element
		nl::node chat = base["chat"];
		if (chat) {
			sprites.emplace_back(chat, DrawArgument(Point<int16_t>(0, 0)));
		}

		// Load additional base textures
		if (base["box"])
			base_box = Texture(base["box"]);
		if (base["chatTarget"])
			base_chatTarget = Texture(base["chatTarget"]);
		if (base["iconBlue"])
			base_iconBlue = Texture(base["iconBlue"]);
		if (base["iconRed"])
			base_iconRed = Texture(base["iconRed"]);
		if (base["iconMemo"])
			base_iconMemo = Texture(base["iconMemo"]);
		if (base["quickSlot"])
			base_quickSlot = Texture(base["quickSlot"]);

		// Load quickslot key textures (Shift, Ins, Hm, Pup, Ctrl, Del, End, Pdn)
		nl::node key_node = statusBar["key"];
		for (int i = 0; i < 8; i++) {
			if (key_node[std::to_string(i)])
				quickslot_keys[i] = Texture(key_node[std::to_string(i)]);
		}

		// Load gauge gray texture
		if (gauge["gray"])
			gauge_gray = Texture(gauge["gray"]);

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
		hpmp_pos = Point<int16_t>(217, 51);     // HP flash position (+28 y)
		mp_pos = Point<int16_t>(217, 51);       // MP flash position (+28 y)
		hpset_pos = Point<int16_t>(236, 41);    // HP value position (-1, -1)
		mpset_pos = Point<int16_t>(348, 42);   // MP value position (0, +8)
		statset_pos = Point<int16_t>(464, 41); // EXP text position (-26, -28 from 490,69)
		levelset_pos = Point<int16_t>(58, 45);  // Level position (-2, -5 from 60, 50)
		namelabel_pos = Point<int16_t>(86, 31);  // Class/player name position (-2, -10 from 88, 41)
		quickslot_pos = Point<int16_t>(VWIDTH - 200, 5); // Quickslot area position (right side)

		// Menu positioning - dynamically calculated based on resolution
		// Base position for 800x600, then offset for larger resolutions
		int16_t menu_base_x = VWIDTH - 118;  // 118px from right edge
		menu_pos = Point<int16_t>(menu_base_x, -280);
		setting_pos = menu_pos + Point<int16_t>(0, 168);
		community_pos = menu_pos + Point<int16_t>(-26, 196);
		character_pos = menu_pos + Point<int16_t>(-61, 168);
		event_pos = menu_pos + Point<int16_t>(-94, 252);

		// Quickslot position - always relative to right edge
		quickslot_pos = Point<int16_t>(VWIDTH - 200, 5);

		// EXP text position is now fixed relative to the centered status bar
		// No dynamic adjustment needed since status bar is centered

		// V92: No separate HP/MP background sprites needed
		// The base background is already drawn, gauges render directly over it
		// Leave hpmp_sprites empty for v92

		// V92: Gauge width for v92 should be based on the actual texture size, not stretching
		// Use a more reasonable width for v92 gauges - they should not stretch across the screen
		int16_t hpmp_max = 120; // V92: Use a consistent, reasonable width that matches texture design

		// Create HP/MP gauges with v92 compatibility
		// V92: Use correct dedicated gauge textures as identified by researcher

		// Verify textures exist before creating gauges
		// Use V87_FILL_RIGHT to clip from right side (shows missing HP/MP portion)
		if (gauge["hpFlash"] && gauge["hpFlash"]["0"]) {
			Texture hpTexture(gauge["hpFlash"]["0"]);
			hpbar = Gauge(Gauge::Type::V87_FILL_RIGHT, hpTexture, hpmp_max, 1.0f);
			// Also load animation for invincibility effect
			hpflash_anim = Animation(gauge["hpFlash"]);
		}

		if (gauge["mpFlash"] && gauge["mpFlash"]["0"]) {
			Texture mpTexture(gauge["mpFlash"]["0"]);
			mpbar = Gauge(Gauge::Type::V87_FILL_RIGHT, mpTexture, hpmp_max, 1.0f);
			// Also load animation for invincibility effect
			mpflash_anim = Animation(gauge["mpFlash"]);
		} 

		// Create character sets with v92 compatibility
		// V92: Use StatusBar number sprites for proper display
		nl::node numbers = statusBar["number"];

		if (numbers) {
			// Create charsets for HP/MP/EXP display using StatusBar number sprites
			statset = Charset(numbers, Charset::Alignment::LEFT);
			hpmpset = Charset(numbers, Charset::Alignment::LEFT);

			// Load numbers from StatusBar/number children
			numset = Charset(numbers, Charset::Alignment::LEFT);

			// Load special character textures directly from StatusBar/number
			// Brackets (green)
			if (numbers["Lbracket"])
				green_lbracket = Texture(numbers["Lbracket"]);
			if (numbers["Rbracket"])
				green_rbracket = Texture(numbers["Rbracket"]);

			// Slash and percent
			if (numbers["slash"])
				white_slash = Texture(numbers["slash"]);
			if (numbers["percent"])
				white_percent = Texture(numbers["percent"]);

			// Dot - may not exist, will just skip if not present
			if (numbers["dot"])
				white_dot = Texture(numbers["dot"]);
		} else {
			// Fallback to default charsets if no number textures exist
			statset = Charset();
			hpmpset = Charset();
			numset = Charset();
		}

		// Load level digit textures from Basic.img/LevelNo (has background)
		nl::node level_node = nl::nx::UI["Basic.img"]["LevelNo"];
		for (int i = 0; i < 10; i++) {
			level_digits[i] = Texture(level_node[std::to_string(i)]);
		}

		// Class name label (first line) and player name label (second line)
		// Using A12M font for smaller text, white color
		classlabel = OutlinedText(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, Color::Name::MINESHAFT);
		namelabel = OutlinedText(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, Color::Name::MINESHAFT);

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
		// Order: Shop, Trade, Menu, Shortcut - all at same y with 2 pixel spacing
		Point<int16_t> v92ButtonPos = Point<int16_t>(VWIDTH - 350, 36);
		int16_t v92_btn_spacing = 56;

		if (statusBar["BtShop"]) {
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(statusBar["BtShop"], v92ButtonPos);
		}
		if (statusBar["BtNPT"]) {
			buttons[Buttons::BT_OPTIONS] = std::make_unique<MapleButton>(statusBar["BtNPT"], v92ButtonPos + Point<int16_t>(v92_btn_spacing, 0));
		}
		if (statusBar["BtMenu"]) {
			buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(statusBar["BtMenu"], v92ButtonPos + Point<int16_t>(v92_btn_spacing * 2, 0));
		}
		if (statusBar["BtShort"]) {
			buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(statusBar["BtShort"], v92ButtonPos + Point<int16_t>(v92_btn_spacing * 3, 0));
		}
			
		// Add hotkey buttons starting at (572, 10), with 2px spacing between each
		// Order: box (with BtClaim inside), EquipKey, InvenKey, StatKey, SkillKey, KeySet, QuickSlot (slides down)
		Point<int16_t> hotkey_start = Point<int16_t>(572, 10);
		int16_t x_offset = 0;
		const int16_t SPACING = 2;

		// base->box setup (texture drawn in draw(), buttons created here)
		// Store the hotkey start position for drawing base_box
		hotkey_box_pos = hotkey_start;

		// BtClaim button - created independently, drawn near iconMemo in draw()
		if (statusBar["BtClaim"]) {
			buttons[Buttons::BT_EVENT] = std::make_unique<MapleButton>(statusBar["BtClaim"], hotkey_start + Point<int16_t>(1, 1));
		}

		if (base_box.is_valid()) {
			x_offset += base_box.get_dimensions().x() + SPACING;
		}

		// Helper lambda to get button width from node
		auto get_button_width = [](nl::node btn_node) -> int16_t {
			// Try different possible texture locations
			nl::node tex_node;
			if (btn_node["normal"]["0"])
				tex_node = btn_node["normal"]["0"];
			else if (btn_node["normal"])
				tex_node = btn_node["normal"];
			else if (btn_node["0"])
				tex_node = btn_node["0"];

			if (tex_node) {
				Texture temp(tex_node);
				if (temp.is_valid())
					return temp.get_dimensions().x();
			}
			return 24;  // fallback width
		};

		// EquipKey button
		if (statusBar["EquipKey"]) {
			buttons[Buttons::BT_EQUIP] = std::make_unique<MapleButton>(statusBar["EquipKey"], hotkey_start + Point<int16_t>(x_offset, 0));
			x_offset += get_button_width(statusBar["EquipKey"]) + SPACING;
		}

		// InvenKey button
		if (statusBar["InvenKey"]) {
			buttons[Buttons::BT_ITEM] = std::make_unique<MapleButton>(statusBar["InvenKey"], hotkey_start + Point<int16_t>(x_offset, 0));
			x_offset += get_button_width(statusBar["InvenKey"]) + SPACING;
		}

		// StatKey button
		if (statusBar["StatKey"]) {
			buttons[Buttons::BT_STAT] = std::make_unique<MapleButton>(statusBar["StatKey"], hotkey_start + Point<int16_t>(x_offset, 0));
			x_offset += get_button_width(statusBar["StatKey"]) + SPACING;
		}

		// SkillKey button
		if (statusBar["SkillKey"]) {
			buttons[Buttons::BT_SKILL] = std::make_unique<MapleButton>(statusBar["SkillKey"], hotkey_start + Point<int16_t>(x_offset, 0));
			x_offset += get_button_width(statusBar["SkillKey"]) + SPACING;
		}

		// KeySet button
		if (statusBar["KeySet"]) {
			buttons[Buttons::BT_KEYSET] = std::make_unique<MapleButton>(statusBar["KeySet"], hotkey_start + Point<int16_t>(x_offset, 0));
			x_offset += get_button_width(statusBar["KeySet"]) + SPACING;
		}

		// QuickSlot toggle button - both textures at same position, visibility depends on quickslot_active
		// When quickslot is hidden: show QuickSlot button (to show quickslot)
		// When quickslot is shown: show QuickSlotD button (to hide quickslot)
		Point<int16_t> qs_btn_pos = hotkey_start + Point<int16_t>(x_offset, 0);
		if (statusBar["QuickSlot"]) {
			buttons[Buttons::BT_QUICKSLOT] = std::make_unique<MapleButton>(statusBar["QuickSlot"], qs_btn_pos);
			buttons[Buttons::BT_QUICKSLOT]->set_active(!quickslot_active);  // Show when quickslot hidden
		}
		if (statusBar["QuickSlotD"]) {
			buttons[Buttons::BT_QUICKSLOT_D] = std::make_unique<MapleButton>(statusBar["QuickSlotD"], qs_btn_pos);
			buttons[Buttons::BT_QUICKSLOT_D]->set_active(quickslot_active);  // Show when quickslot shown
		}
		// Only increment x_offset once since both buttons occupy the same space
		if (statusBar["QuickSlot"] || statusBar["QuickSlotD"]) {
			int16_t qs_width = statusBar["QuickSlot"] ? get_button_width(statusBar["QuickSlot"]) : get_button_width(statusBar["QuickSlotD"]);
			x_offset += qs_width + SPACING;
		}

		// Whisper button
		// commented out as not sure what it does
		//if (statusBar["BtWhisper"]) {
		//	buttons[Buttons::BT_WHISPER] = std::make_unique<MapleButton>(statusBar["BtWhisper"], hotkey_start + Point<int16_t>(x_offset, 0));
		//}

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
		// Order: Shop, Trade, Menu, Shortcut - all at same y with 2 pixel spacing
		constexpr int16_t btn_y = 36;
		constexpr int16_t btn_spacing = 56;  // button width + 2 pixel gap
		int16_t btn_x = 572;  // Shop at (-6, 1) from original

		if (statusBar["BtShop"]) {
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(statusBar["BtShop"], Point<int16_t>(btn_x, btn_y));
			btn_x += btn_spacing;
		}
		if (statusBar["BtNPT"]) {
			buttons[Buttons::BT_OPTIONS] = std::make_unique<MapleButton>(statusBar["BtNPT"], Point<int16_t>(btn_x, btn_y));
			btn_x += btn_spacing;
		}
		if (statusBar["BtMenu"]) {
			buttons[Buttons::BT_MENU] = std::make_unique<MapleButton>(statusBar["BtMenu"], Point<int16_t>(btn_x, btn_y));
			btn_x += btn_spacing;
		}
		if (statusBar["BtShort"]) {
			buttons[Buttons::BT_FOLD_QS] = std::make_unique<MapleButton>(statusBar["BtShort"], Point<int16_t>(btn_x, btn_y));
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
		// Use dynamic height calculation based on resolution
		int16_t statusbar_height = (VWIDTH <= 1024) ? 75 : 80;

		// Center the status bar horizontally based on base texture width
		int16_t center_x = (VWIDTH - base_width) / 2;
		position = Point<int16_t>(center_x, VHEIGHT - statusbar_height + 9);
		position_x = position.x();
		position_y = position.y();
		dimension = Point<int16_t>(base_width, statusbar_height);
	}

	void UIStatusBar::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Draw all main buttons (including v92 hotkeys), except BT_EVENT which is drawn after base_box
		for (size_t i = 0; i <= Buttons::BT_SKILL; i++)
			if (i != Buttons::BT_EVENT && buttons.find(i) != buttons.end() && buttons.at(i))
				buttons.at(i)->draw(position);

		// V92: No HP/MP background sprites needed - they're part of the main background

		// Draw HP/MP flash gauges at fixed positions
		// Flash shows MISSING HP/MP - V87_FILL_RIGHT clips from left to show only missing portion
		// No position offset needed - just draw at fixed position and let clipping handle it
		if (hpbar.is_valid()) {
			hpbar.draw(DrawArgument(position + hpmp_pos, 1.0f));
		}
		if (mpbar.is_valid()) {
			mpbar.draw(DrawArgument(position + mp_pos + Point<int16_t>(111, 0), 1.0f));  // MP bar is 111px right of HP
		}
		if (expbar.is_valid())
			expbar.draw(position + exp_pos);

		int16_t level = stats.get_stat(MapleStat::Id::LEVEL);
		int16_t hp = stats.get_stat(MapleStat::Id::HP);
		int16_t mp = stats.get_stat(MapleStat::Id::MP);
		int32_t maxhp = stats.get_total(EquipStat::Id::HP);
		int32_t maxmp = stats.get_total(EquipStat::Id::MP);
		int64_t exp = stats.get_exp();

		// Format: [ currenthp/maxhp ] with green brackets
		// Draw HP: green [ + white hp + white / + white maxhp + green ]
		// HP numbers offset by (0, 1), brackets at base position
		Point<int16_t> hp_draw_pos = position + hpset_pos;
		Point<int16_t> hp_num_offset = Point<int16_t>(0, 1);  // HP text only offset
		int16_t hp_offset = 0;
		green_lbracket.draw(hp_draw_pos + Point<int16_t>(hp_offset, 0));
		hp_offset += green_lbracket.width() + 1;
		hp_offset += numset.draw(std::to_string(hp), hp_draw_pos + Point<int16_t>(hp_offset, 0) + hp_num_offset);
		white_slash.draw(hp_draw_pos + Point<int16_t>(hp_offset, 0) + hp_num_offset);
		hp_offset += white_slash.width() + 1;  // space after slash
		hp_offset += numset.draw(std::to_string(maxhp), hp_draw_pos + Point<int16_t>(hp_offset, 0) + hp_num_offset);
		hp_offset += 1;
		green_rbracket.draw(hp_draw_pos + Point<int16_t>(hp_offset, 0));

		// Format: [ currentMp/maxMp ] with green brackets
		// MP brackets offset by (0, -1), MP numbers offset by (1, 0)
		Point<int16_t> mp_draw_pos = position + mpset_pos;
		Point<int16_t> mp_bracket_offset = Point<int16_t>(0, -1);  // MP brackets -1 y
		Point<int16_t> mp_num_offset = Point<int16_t>(1, 0);  // MP text only offset
		int16_t mp_offset = 0;
		green_lbracket.draw(mp_draw_pos + Point<int16_t>(mp_offset, 0) + mp_bracket_offset);
		mp_offset += green_lbracket.width() + 1;
		mp_offset += numset.draw(std::to_string(mp), mp_draw_pos + Point<int16_t>(mp_offset, 0) + mp_num_offset);
		white_slash.draw(mp_draw_pos + Point<int16_t>(mp_offset, 0) + mp_num_offset);
		mp_offset += white_slash.width() + 1;  // space after slash
		mp_offset += numset.draw(std::to_string(maxmp), mp_draw_pos + Point<int16_t>(mp_offset, 0) + mp_num_offset);
		mp_offset += 1;
		green_rbracket.draw(mp_draw_pos + Point<int16_t>(mp_offset, 0) + mp_bracket_offset);

		// Format: currentXp[2.54%] with green brackets
		// Text parts offset by (0, 1), brackets at base position
		std::string expstring = std::to_string(100 * getexppercent());
		std::string exp_percent = expstring.substr(0, expstring.find('.') + 3); // e.g., "2.54"
		Point<int16_t> exp_draw_pos = position + statset_pos;
		Point<int16_t> exp_num_offset = Point<int16_t>(0, 1);  // text only offset
		int16_t exp_offset = 0;
		exp_offset += numset.draw(std::to_string(exp), exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
		exp_offset += 1;  // space before [
		green_lbracket.draw(exp_draw_pos + Point<int16_t>(exp_offset, 0));
		exp_offset += green_lbracket.width() + 1;  // space after [
		// Draw percentage - if dot texture exists, split and draw with dot; otherwise just draw digits
		size_t dot_pos = exp_percent.find('.');
		if (dot_pos != std::string::npos && white_dot.is_valid()) {
			// Draw digits before dot
			exp_offset += numset.draw(exp_percent.substr(0, dot_pos), exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
			// Draw dot with tight spacing
			exp_offset -= 1;  // less spacing before dot
			white_dot.draw(exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
			exp_offset += white_dot.width() - 1;  // less spacing after dot
			// Draw digits after dot
			exp_offset += numset.draw(exp_percent.substr(dot_pos + 1), exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
		} else if (dot_pos != std::string::npos) {
			// No dot texture - just show integer percentage
			exp_offset += numset.draw(exp_percent.substr(0, dot_pos), exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
		} else {
			exp_offset += numset.draw(exp_percent, exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
		}
		white_percent.draw(exp_draw_pos + Point<int16_t>(exp_offset, 0) + exp_num_offset);
		exp_offset += white_percent.width() + 1;  // space before ]
		green_rbracket.draw(exp_draw_pos + Point<int16_t>(exp_offset, 0));

		// Draw level using Basic.img/LevelNo digit textures (with background)
		std::string level_str = std::to_string(level);
		Point<int16_t> level_draw_pos = position + levelset_pos + Point<int16_t>(-15, 2);
		int16_t level_x_offset = 0;
		for (char c : level_str) {
			int digit = c - '0';
			if (digit >= 0 && digit <= 9) {
				level_digits[digit].draw(level_draw_pos + Point<int16_t>(level_x_offset, 0));
				level_x_offset += level_digits[digit].width();
			}
		}

		// Draw class name (first line) and player name (second line)
		classlabel.draw(position + namelabel_pos);
		namelabel.draw(position + namelabel_pos + Point<int16_t>(0, 13));  // 13px below class name (smaller font)

		// Draw hotkey box (base->box texture) at hotkey_box_pos
		if (base_box.is_valid()) {
			base_box.draw(DrawArgument(position + hotkey_box_pos));
		}

		// Draw BtClaim button ON TOP of base_box
		if (buttons.count(Buttons::BT_EVENT) && buttons.at(Buttons::BT_EVENT)) {
			buttons.at(Buttons::BT_EVENT)->draw(position);

			// Draw iconMemo to the right of BtClaim
			if (base_iconMemo.is_valid()) {
				Rectangle<int16_t> claim_bounds = buttons.at(Buttons::BT_EVENT)->bounds(position);
				Point<int16_t> memo_pos = Point<int16_t>(
					claim_bounds.right() + 4 - position.x(),  // 4px to the right of BtClaim
					hotkey_box_pos.y() + 5                     // 5px from top of box
				);
				base_iconMemo.draw(DrawArgument(position + memo_pos));
			}
		}

		// V92: Draw quickslot (only when active)
		// Get BtShort button bounds to position quickslot relative to it
		if (quickslot_active && base_quickSlot.is_valid() && buttons.count(Buttons::BT_FOLD_QS) && buttons.at(Buttons::BT_FOLD_QS)) {
			Rectangle<int16_t> btn_bounds = buttons.at(Buttons::BT_FOLD_QS)->bounds(position);

			// Quickslot starts 6px right of button, bottom aligned
			// Account for texture origin when calculating position
			Point<int16_t> qs_dims = base_quickSlot.get_dimensions();
			Point<int16_t> qs_origin = base_quickSlot.get_origin();
			Point<int16_t> qs_pos = Point<int16_t>(
				btn_bounds.right() + 6 - position.x() + qs_origin.x() + 0,
				btn_bounds.bottom() - qs_dims.y() - position.y() + qs_origin.y() + 3
			);

			// Draw quickslot background
			base_quickSlot.draw(DrawArgument(position + qs_pos));

			// Draw skill/item icons on quickslots FIRST (so they appear behind key labels)
			draw_quickslot_icons(qs_pos);

			// Draw key labels ON TOP: 4 columns x 2 rows, each slot is 35x35
			// Labels start at offset (10, 9) from quickslot top-left
			int16_t slot_size = 35;
			Point<int16_t> key_start = qs_pos + Point<int16_t>(10, 9);

			for (int i = 0; i < 8; i++) {
				if (quickslot_keys[i].is_valid()) {
					int row = i / 4;
					int col = i % 4;
					Point<int16_t> key_pos = key_start + Point<int16_t>(col * slot_size, row * slot_size);
					quickslot_keys[i].draw(DrawArgument(position + key_pos));
				}
			}
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
		// Flash shows MISSING HP/MP, so invert the percentage
		if (hpbar.is_valid())
			hpbar.update(1.0f - gethppercent());
		if (mpbar.is_valid())
			mpbar.update(1.0f - getmppercent());

		classlabel.change_text(stats.get_jobname());
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
				// V92: BT_FOLD_QS is BtShort (Shortcut button) - opens key config
				UI::get().emplace<UIKeyConfig>(
					Stage::get().get_player().get_inventory(),
					Stage::get().get_player().get_skills()
				);
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
			case Buttons::BT_KEYSET:
			{
				UI::get().emplace<UIKeyConfig>(
					Stage::get().get_player().get_inventory(),
					Stage::get().get_player().get_skills()
				);
				break;
			}
			case Buttons::BT_QUICKSLOT:
		case Buttons::BT_QUICKSLOT_D:
			{
				toggle_qs();
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

		// Toggle quickslot button visibility (same position, swap which is shown)
		if (buttons.find(Buttons::BT_QUICKSLOT) != buttons.end())
			buttons[Buttons::BT_QUICKSLOT]->set_active(!quickslot_active);  // Show when qs hidden
		if (buttons.find(Buttons::BT_QUICKSLOT_D) != buttons.end())
			buttons[Buttons::BT_QUICKSLOT_D]->set_active(quickslot_active);  // Show when qs shown
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

	UIElement::ComponentInfo UIStatusBar::get_component_at(Point<int16_t> cursor_position) const
	{
		ComponentInfo info;

		// HP bar area (approximate bounds based on hpmp_pos)
		Rectangle<int16_t> hp_bounds(
			position + hpmp_pos,
			position + hpmp_pos + Point<int16_t>(100, 12)
		);
		if (hp_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "HP Bar (Gauge)";
			info.position = position + hpmp_pos;
			info.dimension = Point<int16_t>(100, 12);
			return info;
		}

		// MP bar area (below HP bar)
		Rectangle<int16_t> mp_bounds(
			position + mp_pos,
			position + mp_pos + Point<int16_t>(100, 12)
		);
		if (mp_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "MP Bar (Gauge)";
			info.position = position + mp_pos;
			info.dimension = Point<int16_t>(100, 12);
			return info;
		}

		// EXP bar area
		Rectangle<int16_t> exp_bounds(
			position + exp_pos,
			position + exp_pos + Point<int16_t>(VWIDTH, 8)
		);
		if (exp_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "EXP Bar (Gauge)";
			info.position = position + exp_pos;
			info.dimension = Point<int16_t>(VWIDTH, 8);
			return info;
		}

		// Level display area
		Rectangle<int16_t> level_bounds(
			position + levelset_pos - Point<int16_t>(20, 10),
			position + levelset_pos + Point<int16_t>(40, 20)
		);
		if (level_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "Level Display";
			info.position = position + levelset_pos;
			info.dimension = Point<int16_t>(40, 20);
			return info;
		}

		// Class/Name label area (two lines: class name + player name)
		Rectangle<int16_t> name_bounds(
			position + namelabel_pos - Point<int16_t>(5, 5),
			position + namelabel_pos + Point<int16_t>(80, 35)  // Taller to accommodate two lines
		);
		if (name_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "Class & Player Name";
			info.position = position + namelabel_pos;
			info.dimension = Point<int16_t>(80, 32);
			return info;
		}

		// Quickslot area
		Rectangle<int16_t> qs_bounds(
			position + quickslot_pos,
			position + quickslot_pos + Point<int16_t>(200, 40)
		);
		if (qs_bounds.contains(cursor_position))
		{
			info.type = ComponentInfo::SPRITE;
			info.name = "Quickslot Area";
			info.position = position + quickslot_pos;
			info.dimension = Point<int16_t>(200, 40);
			return info;
		}

		// Fall back to button detection from parent
		return UIElement::get_component_at(cursor_position);
	}

	void UIStatusBar::draw_quickslot_icons(Point<int16_t> qs_pos) const
	{
		// Quickslot key codes for binding lookup
		// Row 0: Shift(42), Ins(82), Home(71), PageUp(73)
		// Row 1: Ctrl(29), Del(83), End(79), PageDown(81)
		static const int32_t QUICKSLOT_KEYCODES[8] = {
			KeyConfig::LEFT_SHIFT, KeyConfig::INSERT, KeyConfig::HOME, KeyConfig::PAGE_UP,
			KeyConfig::LEFT_CONTROL, KeyConfig::DELETE, KeyConfig::END, KeyConfig::PAGE_DOWN
		};

		// Get keyboard for key mappings
		Keyboard& keyboard = UI::get().get_keyboard();

		// Get inventory for item counts
		const Inventory& inventory = Stage::get().get_player().get_inventory();

		// Slot layout: 4 columns x 2 rows, each slot is 35x35
		// Icons should be centered in each slot
		int16_t slot_size = 35;
		int16_t icon_size = 32;  // Standard icon size
		int16_t icon_offset = (slot_size - icon_size) / 2;

		// Icon start position (relative to quickslot top-left, below the key labels)
		// Offset by (-1, 6) to position icons below key labels
		Point<int16_t> icon_start = qs_pos + Point<int16_t>(10 + icon_offset - 4, 13 + icon_offset + 25);

		for (int i = 0; i < 8; i++)
		{
			int32_t keycode = QUICKSLOT_KEYCODES[i];
			Keyboard::Mapping mapping = keyboard.get_maple_mapping(keycode);

			if (mapping.type == KeyType::Id::NONE)
				continue;

			int row = i / 4;
			int col = i % 4;
			Point<int16_t> icon_pos = position + icon_start + Point<int16_t>(col * slot_size, row * slot_size);

			if (mapping.type == KeyType::Id::ITEM && mapping.action > 0)
			{
				// Draw item icon
				Texture icon = get_item_texture(mapping.action);
				if (icon.is_valid())
				{
					icon.draw(DrawArgument(icon_pos));

					// Draw item count using ItemNo charset (same as Icon.cpp)
					// Account for icon origin to position count correctly relative to visual icon
					// Always show count (including 0) so player knows when items run out
					int16_t count = inventory.get_total_item_count(mapping.action);
					static const Charset countset = Charset(nl::nx::UI["Basic.img"]["ItemNo"], Charset::Alignment::LEFT);
					Point<int16_t> origin = icon.get_origin();
					Point<int16_t> count_pos = icon_pos - origin + Point<int16_t>(2, icon.get_dimensions().y() - 12);
					countset.draw(std::to_string(count), count_pos);
				}
			}
			else if (mapping.type == KeyType::Id::SKILL && mapping.action > 0)
			{
				// Draw skill icon
				Texture icon = get_skill_texture(mapping.action);
				if (icon.is_valid())
				{
					icon.draw(DrawArgument(icon_pos));
				}
			}
		}
	}

	Texture UIStatusBar::get_item_texture(int32_t item_id) const
	{
		const ItemData& data = ItemData::get(item_id);
		return data.get_icon(false);
	}

	Texture UIStatusBar::get_skill_texture(int32_t skill_id) const
	{
		const SkillData& data = SkillData::get(skill_id);
		return data.get_icon(SkillData::Icon::NORMAL);
	}

	Point<int16_t> UIStatusBar::get_quickslot_icon_start() const
	{
		// Calculate quickslot icon start position based on BtShort button bounds
		if (base_quickSlot.is_valid() && buttons.count(Buttons::BT_FOLD_QS) && buttons.at(Buttons::BT_FOLD_QS)) {
			Rectangle<int16_t> btn_bounds = buttons.at(Buttons::BT_FOLD_QS)->bounds(position);

			Point<int16_t> qs_dims = base_quickSlot.get_dimensions();
			Point<int16_t> qs_origin = base_quickSlot.get_origin();
			Point<int16_t> qs_pos = Point<int16_t>(
				btn_bounds.right() + 6 - position.x() + qs_origin.x() + 0,
				btn_bounds.bottom() - qs_dims.y() - position.y() + qs_origin.y() + 3
			);

			// Icon layout constants (matching draw_quickslot_icons)
			int16_t slot_size = 35;
			int16_t icon_size = 32;
			int16_t icon_offset = (slot_size - icon_size) / 2;

			return qs_pos + Point<int16_t>(10 + icon_offset, 9 + icon_offset);
		}
		return Point<int16_t>(0, 0);
	}

	int16_t UIStatusBar::quickslot_by_position(Point<int16_t> cursorpos) const
	{
		Point<int16_t> icon_start = get_quickslot_icon_start();
		if (icon_start.x() == 0 && icon_start.y() == 0)
			return -1;

		Point<int16_t> abs_icon_start = position + icon_start;

		// Slot layout: 4 columns x 2 rows, each slot is 35x35
		int16_t slot_size = 35;
		int16_t icon_size = 32;

		// Calculate relative position from icon start
		int16_t rel_x = cursorpos.x() - abs_icon_start.x();
		int16_t rel_y = cursorpos.y() - abs_icon_start.y();

		// Check bounds
		if (rel_x < 0 || rel_y < 0)
			return -1;
		if (rel_x >= 4 * slot_size || rel_y >= 2 * slot_size)
			return -1;

		// Calculate row and column
		int col = rel_x / slot_size;
		int row = rel_y / slot_size;

		if (col >= 4 || row >= 2)
			return -1;

		// Return slot index (0-7)
		return static_cast<int16_t>(row * 4 + col);
	}

	int32_t UIStatusBar::get_quickslot_keycode(int16_t slot) const
	{
		// Quickslot key codes:
		// Row 0: Shift(42), Ins(82), Home(71), PageUp(73)
		// Row 1: Ctrl(29), Del(83), End(79), PageDown(81)
		static const int32_t QUICKSLOT_KEYCODES[8] = {
			KeyConfig::LEFT_SHIFT, KeyConfig::INSERT, KeyConfig::HOME, KeyConfig::PAGE_UP,
			KeyConfig::LEFT_CONTROL, KeyConfig::DELETE, KeyConfig::END, KeyConfig::PAGE_DOWN
		};

		if (slot >= 0 && slot < 8)
			return QUICKSLOT_KEYCODES[slot];

		return -1;
	}

	bool UIStatusBar::send_icon(const Icon& icon, Point<int16_t> cursor_position)
	{
		// Check if cursor is over a quickslot
		int16_t slot = quickslot_by_position(cursor_position);

		if (slot >= 0 && slot < 8)
		{
			// Get the keycode for this quickslot
			int32_t keycode = get_quickslot_keycode(slot);

			if (keycode >= 0)
			{
				// Get icon type to determine what kind of binding to create
				Icon::IconType icon_type = const_cast<Icon&>(icon).get_type();

				if (icon_type == Icon::IconType::ITEM || icon_type == Icon::IconType::SKILL)
				{
					// Get the action ID from the icon
					int32_t action_id = icon.get_action_id();

					if (action_id > 0)
					{
						// Block targeted consumables (scrolls) from quickslots
						// Scrolls have item prefix 204xxxx (item_id / 10000 == 204)
						if (icon_type == Icon::IconType::ITEM)
						{
							int32_t item_prefix = action_id / 10000;
							// 204: Upgrade scrolls, 261: Potential scrolls, 206: Arrows (not usable directly)
							if (item_prefix == 204 || item_prefix == 261)
							{
								// Play error sound and reject
								Sound(Sound::Name::DLGNOTICE).play();
								return true;  // Consume the icon but don't bind
							}
						}

						// Determine the key type based on icon type
						KeyType::Id key_type = (icon_type == Icon::IconType::SKILL) ?
							KeyType::Id::SKILL : KeyType::Id::ITEM;

						// Update the keyboard binding locally
						Keyboard& keyboard = UI::get().get_keyboard();
						keyboard.assign(static_cast<uint8_t>(keycode), static_cast<uint8_t>(key_type), action_id);

						// Send packet to server to persist the binding
						KeyConfig::Key config_key = static_cast<KeyConfig::Key>(keycode);
						std::vector<std::tuple<KeyConfig::Key, KeyType::Id, int32_t>> updated_actions;
						updated_actions.emplace_back(std::make_tuple(config_key, key_type, action_id));
						ChangeKeyMapPacket(updated_actions).dispatch();

						// Play drop sound
						Sound(Sound::Name::DRAGEND).play();

						return true;  // Icon consumed
					}
				}
			}
		}

		return true;  // Always consume to prevent falling through
	}

	Cursor::State UIStatusBar::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Check for hover/click on quickslots (future: could implement drag-out)
		int16_t slot = quickslot_by_position(cursorpos);

		if (slot >= 0 && slot < 8)
		{
			// If there's a binding on this slot, show grab cursor
			int32_t keycode = get_quickslot_keycode(slot);
			if (keycode >= 0)
			{
				Keyboard& keyboard = UI::get().get_keyboard();
				Keyboard::Mapping mapping = keyboard.get_maple_mapping(keycode);

				if (mapping.type != KeyType::Id::NONE && mapping.action > 0)
				{
					// Has a binding - show can grab cursor
					if (clicked)
					{
						// Future: implement drag-out to remove binding
						return Cursor::State::CLICKING;
					}
					return Cursor::State::CANGRAB;
				}
			}
		}

		// Default behavior
		return UIElement::send_cursor(clicked, cursorpos);
	}
}