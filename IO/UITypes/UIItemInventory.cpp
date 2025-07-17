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
#include "UIItemInventory.h"

#include "UINotice.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../UITypes/UIKeyConfig.h"

#include "../../Data/EquipData.h"
#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/InventoryPackets.h"
#include "../../Util/V83UIAssets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIItemInventory::UIItemInventory(const Inventory& invent) : UIDragElement<PosINV>(), inventory(invent), ignore_tooltip(false), tab(InventoryType::Id::EQUIP), sort_enabled(true)
	{
		LOG(LOG_DEBUG, "[UIItemInventory] Constructor START");
		
		// Use version detection
		bool is_v83 = V83UIAssets::isV83Mode();
		bool is_v92 = V83UIAssets::isV92Mode();
		
		nl::node Item;
		if (is_v83) {
			// V83/V87 structure
			Item = nl::nx::UI["UIWindow.img"]["Item"];
			LOG(LOG_DEBUG, "[UIItemInventory] Using v83/v87 UI structure");
		} else if (is_v92) {
			// V92 structure - uses UIWindow.img but has tabs
			Item = nl::nx::UI["UIWindow.img"]["Item"];
			LOG(LOG_DEBUG, "[UIItemInventory] Using v92 UI structure");
		} else {
			// Modern structure
			Item = nl::nx::UI["UIWindow2.img"]["Item"];
			LOG(LOG_DEBUG, "[UIItemInventory] Using modern UI structure");
		}
		
		if (Item.name().empty()) {
			LOG(LOG_ERROR, "[UIItemInventory] Item node not found!");
		}
		
		// Debug: List all child nodes of Item
		LOG(LOG_DEBUG, "[UIItemInventory] Available Item child nodes:");
		for (auto child : Item) {
			LOG(LOG_DEBUG, "  - " << child.name() << " (type: " << (int)child.data_type() << ")");
		}
		
		// Handle position data based on version
		if (is_v83) {
			// V83/V87 defaults - no position data in NX files
			LOG(LOG_DEBUG, "[UIItemInventory] Using v83/v87 defaults - no pos data in NX");
			slot_col = 4;
			slot_pos = Point<int16_t>(11, 51); // Position for first slot in v87
			slot_row = 6;
			slot_space_x = 36;
			slot_space_y = 35;
		} else if (is_v92) {
			// V92 uses same defaults as v83/v87 but has tabs
			LOG(LOG_DEBUG, "[UIItemInventory] Using v92 defaults - similar to v87 but with tabs");
			slot_col = 4;
			slot_pos = Point<int16_t>(11, 51); 
			slot_row = 6;
			slot_space_x = 36;
			slot_space_y = 35;
		} else {
			// Modern MapleStory layout
			nl::node pos = Item["pos"];
			slot_col = pos["slot_col"];
			slot_pos = pos["slot_pos"];
			slot_row = pos["slot_row"];
			slot_space_x = pos["slot_space_x"];
			slot_space_y = pos["slot_space_y"];
		}
		
		// Validate slot_pos
		if (slot_pos.x() == 0 && slot_pos.y() == 0) {
			LOG(LOG_DEBUG, "[UIItemInventory] slot_pos is (0,0), using default");
			slot_pos = Point<int16_t>(11, 51); // Default position for v87
		}
		LOG(LOG_DEBUG, "[UIItemInventory] slot_pos=" << slot_pos.x() << "," << slot_pos.y());

		// Initialize with default values
		max_slots = 24;
		max_full_slots = 96;

		// These nodes don't exist in v83/v87, and may not exist in v92
		nl::node AutoBuild = (is_v83 || is_v92) ? nl::node() : Item["AutoBuild"];
		nl::node FullAutoBuild = (is_v83 || is_v92) ? nl::node() : Item["FullAutoBuild"];

		// Load backgrounds - simplified to use only main background
		nl::node backgrnd_node = Item["backgrnd"];
		if (backgrnd_node && backgrnd_node.data_type() == nl::node::type::bitmap) {
			backgrnd = backgrnd_node;
			LOG(LOG_DEBUG, "[UIItemInventory] Loaded backgrnd");
		} else {
			// Fallback for modern versions that might use productionBackgrnd
			backgrnd_node = Item["productionBackgrnd"];
			if (backgrnd_node && backgrnd_node.data_type() == nl::node::type::bitmap) {
				backgrnd = backgrnd_node;
				LOG(LOG_DEBUG, "[UIItemInventory] Loaded productionBackgrnd as fallback");
			} else {
				LOG(LOG_ERROR, "[UIItemInventory] No background found!");
				backgrnd = Texture();
			}
		}
		
		// Don't load backgrnd2/3 - not needed
		backgrnd2 = Texture();
		backgrnd3 = Texture();
		
		// Load full background if available
		nl::node full_bg_node = Item["FullBackgrnd"];
		if (full_bg_node && full_bg_node.data_type() == nl::node::type::bitmap) {
			full_backgrnd = full_bg_node;
			LOG(LOG_DEBUG, "[UIItemInventory] Loaded FullBackgrnd");
		} else {
			// Use regular background as fallback for full view
			full_backgrnd = backgrnd;
			LOG(LOG_DEBUG, "[UIItemInventory] Using backgrnd as full_backgrnd fallback");
		}
		
		// Don't load full_backgrnd2/3 - not needed
		full_backgrnd2 = Texture();
		full_backgrnd3 = Texture();

		bg_dimensions = backgrnd.get_dimensions();
		bg_full_dimensions = full_backgrnd.get_dimensions();

		// V83/V87 doesn't have these assets, v92 might have some
		if (is_v83) {
			// Use empty textures for v83/v87
			newitemslot = Animation();
			newitemtabdis = Animation();
			newitemtaben = Animation();
			projectile = Texture();
			disabled = Texture();
		} else if (is_v92) {
			// V92 has basic assets like modern versions
			nl::node New = Item["New"];
			if (New) {
				newitemslot = New["inventory"];
				newitemtabdis = New["Tab0"];
				newitemtaben = New["Tab1"];
			} else {
				newitemslot = Animation();
				newitemtabdis = Animation();
				newitemtaben = Animation();
			}
			projectile = Item["activeIcon"];
			disabled = Item["disabled"];
		} else {
			// Modern versions have all assets
			nl::node New = Item["New"];
			newitemslot = New["inventory"];
			newitemtabdis = New["Tab0"];
			newitemtaben = New["Tab1"];
			projectile = Item["activeIcon"];
			disabled = Item["disabled"];
		}

		Point<int16_t> icon_dimensions = disabled.get_dimensions();
		icon_width = icon_dimensions.x();
		icon_height = icon_dimensions.y();

		// Handle tabs based on version
		if (is_v92) {
			// V92 has 5 tabs (0-4) in UIWindow.img
			nl::node Tab = Item["Tab"];
			nl::node taben = Tab["enabled"];
			nl::node tabdis = Tab["disabled"];

			// v92: Since origins are (0,0), calculate positions manually
			// Tabs should be positioned horizontally across the top of the inventory
			// Base position for first tab, then space them out according to actual tab widths
			Point<int16_t> tab_base = Point<int16_t>(9, 26);
			
			// Calculate positions with proper spacing to prevent overlap
			// Tab widths from JSON: 22, 13, 26, 13, 20 - add 2px padding between tabs
			Point<int16_t> tab_pos0 = tab_base;                                  // Equip: x=9
			Point<int16_t> tab_pos1 = tab_base + Point<int16_t>(26, 0);         // Use: x=35 (22+4 spacing)
			Point<int16_t> tab_pos2 = tab_base + Point<int16_t>(43, 0);         // ETC: x=52 (26+13+4 spacing)
			Point<int16_t> tab_pos3 = tab_base + Point<int16_t>(73, 0);         // Setup: x=82 (43+26+4 spacing)  
			Point<int16_t> tab_pos4 = tab_base + Point<int16_t>(90, 0);         // Cash: x=99 (73+13+4 spacing)
			
			// Create tab buttons only if textures are valid
			if (tabdis["0"] && taben["0"]) {
				buttons[Buttons::BT_TAB_EQUIP] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"], tab_pos0);
			}
			if (tabdis["1"] && taben["1"]) {
				buttons[Buttons::BT_TAB_USE] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"], tab_pos1);
			}
			if (tabdis["2"] && taben["2"]) {
				buttons[Buttons::BT_TAB_ETC] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"], tab_pos2);
			}
			if (tabdis["3"] && taben["3"]) {
				buttons[Buttons::BT_TAB_SETUP] = std::make_unique<TwoSpriteButton>(tabdis["3"], taben["3"], tab_pos3);
			}
			if (tabdis["4"] && taben["4"]) {
				buttons[Buttons::BT_TAB_CASH] = std::make_unique<TwoSpriteButton>(tabdis["4"], taben["4"], tab_pos4);
			}
			// V92 doesn't have tab 5 (BT_TAB_DEC) - skip it
			LOG(LOG_DEBUG, "[UIItemInventory] V92 mode - loaded 5 tabs (0-4), no tab 5");
		} else if (!is_v83) {
			// Modern versions have 6 tabs (0-5)
			nl::node Tab = Item["Tab"];
			nl::node taben = Tab["enabled"];
			nl::node tabdis = Tab["disabled"];

			Point<int16_t> tab_pos0 = Texture(taben["0"]).get_origin() * -1;
			Point<int16_t> tab_pos1 = Texture(taben["1"]).get_origin() * -1;
			Point<int16_t> tab_pos2 = Texture(taben["2"]).get_origin() * -1;
			Point<int16_t> tab_pos3 = Texture(taben["3"]).get_origin() * -1;
			Point<int16_t> tab_pos4 = Texture(taben["4"]).get_origin() * -1;
			Point<int16_t> tab_pos5 = Texture(taben["5"]).get_origin() * -1;
			Point<int16_t> tab_pos_adj = Point<int16_t>(9, 26);
			
			buttons[Buttons::BT_TAB_EQUIP] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"], tab_pos0 - tab_pos_adj, Point<int16_t>(0, 0));
			buttons[Buttons::BT_TAB_USE] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"], tab_pos1 - tab_pos_adj, Point<int16_t>(0, 0));
			buttons[Buttons::BT_TAB_ETC] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"], tab_pos2 - tab_pos_adj, Point<int16_t>(0, 0));
			buttons[Buttons::BT_TAB_SETUP] = std::make_unique<TwoSpriteButton>(tabdis["3"], taben["3"], tab_pos3 - tab_pos_adj, Point<int16_t>(0, 0));
			buttons[Buttons::BT_TAB_CASH] = std::make_unique<TwoSpriteButton>(tabdis["4"], taben["4"], tab_pos4 - tab_pos_adj, Point<int16_t>(0, 0));
			buttons[Buttons::BT_TAB_DEC] = std::make_unique<TwoSpriteButton>(tabdis["5"], taben["5"], tab_pos5 - tab_pos_adj, Point<int16_t>(0, 0));
			LOG(LOG_DEBUG, "[UIItemInventory] Modern mode - loaded 6 tabs (0-5)");
		} else {
			// V83/V87 has no tab system - create dummy buttons
			LOG(LOG_DEBUG, "[UIItemInventory] V83/V87 mode - no tabs");
		}

		// Close button exists in both versions
		nl::node close = nl::nx::UI["Basic.img"]["BtClose3"];
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close);

		// V83/V87 and V92 don't have these AutoBuild buttons
		if (!is_v83 && !is_v92) {
			buttons[Buttons::BT_COIN] = std::make_unique<MapleButton>(AutoBuild["button:Coin"]);
			buttons[Buttons::BT_POINT] = std::make_unique<MapleButton>(AutoBuild["button:Point"]);
			buttons[Buttons::BT_GATHER] = std::make_unique<MapleButton>(AutoBuild["button:Gather"]);
			buttons[Buttons::BT_SORT] = std::make_unique<MapleButton>(AutoBuild["button:Sort"]);
			buttons[Buttons::BT_FULL] = std::make_unique<MapleButton>(AutoBuild["button:Full"]);
			buttons[Buttons::BT_UPGRADE] = std::make_unique<MapleButton>(AutoBuild["button:Upgrade"]);
			buttons[Buttons::BT_APPRAISE] = std::make_unique<MapleButton>(AutoBuild["button:Appraise"]);
			buttons[Buttons::BT_EXTRACT] = std::make_unique<MapleButton>(AutoBuild["button:Extract"]);
			buttons[Buttons::BT_DISASSEMBLE] = std::make_unique<MapleButton>(AutoBuild["button:Disassemble"]);
			buttons[Buttons::BT_TOAD] = std::make_unique<MapleButton>(AutoBuild["anibutton:Toad"]);

			buttons[Buttons::BT_SMALL] = std::make_unique<MapleButton>(FullAutoBuild["button:Small"]);
			buttons[Buttons::BT_COIN_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Coin"]);
			buttons[Buttons::BT_POINT_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Point"]);
			buttons[Buttons::BT_GATHER_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Gather"]);
			buttons[Buttons::BT_SORT_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Sort"]);
			buttons[Buttons::BT_UPGRADE_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Upgrade"]);
			buttons[Buttons::BT_APPRAISE_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Appraise"]);
			buttons[Buttons::BT_EXTRACT_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Extract"]);
			buttons[Buttons::BT_DISASSEMBLE_SM] = std::make_unique<MapleButton>(FullAutoBuild["button:Disassemble"]);
			buttons[Buttons::BT_TOAD_SM] = std::make_unique<MapleButton>(FullAutoBuild["anibutton:Toad"]);
			buttons[Buttons::BT_CASHSHOP] = std::make_unique<MapleButton>(FullAutoBuild["button:Cashshop"]);
			
			buttons[Buttons::BT_EXTRACT]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_EXTRACT_SM]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_DISASSEMBLE]->set_state(Button::State::DISABLED);
			buttons[Buttons::BT_DISASSEMBLE_SM]->set_state(Button::State::DISABLED);
		} else {
			// V83/V87 only has minimal buttons - maybe sort button in Item window
			LOG(LOG_DEBUG, "[UIItemInventory] V83/V87 mode - minimal buttons");
		}

		// Set initial tab button state for all versions that have tabs (v92 and modern)
		if (is_v92 || (!is_v83)) {
			// For v92, explicitly activate all tab buttons
			if (is_v92) {
				if (buttons[Buttons::BT_TAB_EQUIP]) buttons[Buttons::BT_TAB_EQUIP]->set_active(true);
				if (buttons[Buttons::BT_TAB_USE]) buttons[Buttons::BT_TAB_USE]->set_active(true);
				if (buttons[Buttons::BT_TAB_ETC]) buttons[Buttons::BT_TAB_ETC]->set_active(true);
				if (buttons[Buttons::BT_TAB_SETUP]) buttons[Buttons::BT_TAB_SETUP]->set_active(true);
				if (buttons[Buttons::BT_TAB_CASH]) buttons[Buttons::BT_TAB_CASH]->set_active(true);
				// Don't activate BT_TAB_DEC for v92 since it doesn't exist
			}
			
			uint16_t tab_btn = button_by_tab(tab);
			if (tab_btn < buttons.size() && buttons[tab_btn])
				buttons[tab_btn]->set_state(Button::State::PRESSED);
		}

		// Moved into the !is_v87 block above

		mesolabel = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::BLACK);
		maplepointslabel = Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::BLACK);
		maplepointslabel.change_text("0"); // TODO: Implement

		for (size_t i = 0; i < InventoryType::Id::LENGTH; i++)
		{
			InventoryType::Id id = InventoryType::by_value(i);
			slotrange[id] = std::pair<int16_t, int16_t>(1, 24);
		}

		// Add safety check for inventory slot access
		int16_t slotmax = 0;
		try {
			slotmax = inventory.get_slotmax(tab);
		} catch (...) {
			slotmax = 24; // Default to 24 slots if inventory access fails
		}
		
		if (slotmax <= 0) {
			slotmax = 24; // Ensure we have a positive slot count
		}
		
		// Add safety checks for all variables used in calculations
		if (slot_col <= 0) slot_col = 4;  // Default to 4 columns
		if (icon_height <= 0) icon_height = 32;  // Default icon height
		if (slot_space_y <= 0) slot_space_y = 35;  // Default Y spacing
		
		int16_t second = (icon_height + slot_space_y) * slotmax / slot_col + 24;
		// Add safety checks for remaining variables
		if (icon_width <= 0) icon_width = 32;  // Default icon width
		if (slot_space_x <= 0) slot_space_x = 36;  // Default X spacing  
		if (slot_row <= 2) slot_row = 6;  // Default to 6 rows minimum
		
		// Recalculate slot counts after safety checks
		max_slots = slot_row * slot_col;
		max_full_slots = slot_col * max_slots;
		
		
		int16_t x = slot_col * (icon_width + slot_space_x) + 4;
		int16_t unitrows = slot_row - 2;
		if (unitrows <= 0) unitrows = 1; // Prevent negative or zero rows
		
		int16_t rowmax = (slot_col > 0) ? (slotmax / slot_col) : 1;
		if (rowmax <= 0) rowmax = 1; // Prevent negative or zero rowmax

		LOG(LOG_DEBUG, "[UIItemInventory] Creating slider with params:");
		LOG(LOG_DEBUG, "  slot_pos.y()=" << slot_pos.y() << ", second=" << second);
		LOG(LOG_DEBUG, "  x=" << x << ", unitrows=" << unitrows << ", rowmax=" << rowmax);
		LOG(LOG_DEBUG, "  slotmax=" << slotmax << ", slot_col=" << slot_col);

		// V83 doesn't need a slider - inventory is fixed size
		// V92 and modern versions can use a slider
		if (is_v92) {
			// V92: Create a functional slider with appropriate defaults
			// Position slider on the right side of inventory, within window bounds
			int16_t slider_x = 165;  // Right side of 175px wide inventory
			int16_t slider_y_start = slot_pos.y();
			int16_t slider_y_end = 250;  // Keep within 289px inventory height
			
			// Ensure slider doesn't overflow - limit to inventory bounds
			if (slider_y_end > 280) slider_y_end = 280;
			
			slider = Slider(
				Slider::Type::LINE_CYAN, Range<int16_t>(slider_y_start, slider_y_end), slider_x, 4, 6,
				[&](bool upwards)
				{
					int16_t shift = upwards ? -slot_col : slot_col;
					slotrange[tab].first += shift;
					slotrange[tab].second += shift;
				}
			);
		} else if (!is_v83 && slot_pos.y() > 0 && second > slot_pos.y() && x > 0 && unitrows > 0 && rowmax > 0) {
			slider = Slider(
				Slider::Type::DEFAULT_SILVER, Range<int16_t>(slot_pos.y(), second), x, unitrows, rowmax,
				[&](bool upwards)
				{
					int16_t shift = upwards ? -slot_col : slot_col;

					slotrange[tab].first += shift;
					slotrange[tab].second += shift;
				}
			);
		} else {
			LOG(LOG_DEBUG, "[UIItemInventory] V83 mode or invalid params - no slider");
			// Create a disabled slider
			slider = Slider();
		}

#if LOG_LEVEL >= LOG_UI
		for (size_t i = 0; i < max_full_slots; i++)
			slot_labels[i] = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, std::to_string(i + 1));
#endif

		// Defer heavy initialization to first update() call to avoid UI toggle deadlock
		needs_init = true;
		full_enabled = false;  // Set initial state without calling set_full()
		clear_new();
		LOG(LOG_DEBUG, "[UIItemInventory] Constructor END - needs_init=" << needs_init << ", full_enabled=" << full_enabled);
	}

	void UIItemInventory::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Use version-aware positioning for mesos text
		Point<int16_t> mesolabel_pos;
		Point<int16_t> maplepointslabel_pos;
		
		bool is_v92 = V83UIAssets::isV92Mode();
		if (is_v92) {
			// V92 inventory background is 175x289, so position mesos text within it
			// Position at bottom of inventory with some padding
			mesolabel_pos = position + Point<int16_t>(15, 265);
			maplepointslabel_pos = position + Point<int16_t>(95, 265);
		} else {
			// Modern MapleStory positions
			mesolabel_pos = position + Point<int16_t>(144, 305);
			maplepointslabel_pos = position + Point<int16_t>(179, 323);
		}

		if (full_enabled)
		{
			if (full_backgrnd.is_valid()) {
				full_backgrnd.draw(position);
			}

			mesolabel.draw(mesolabel_pos + Point<int16_t>(0, 84));
			maplepointslabel.draw(maplepointslabel_pos + Point<int16_t>(220, 66));
		}
		else
		{
			// Only draw the main background
			if (backgrnd.is_valid()) {
				backgrnd.draw(position);
			}
			
			// Draw slider if it's enabled and valid
			bool is_v92 = V83UIAssets::isV92Mode();
			if (is_v92 && slider.isenabled()) {
				slider.draw(position);
			}

			mesolabel.draw(mesolabel_pos);
			maplepointslabel.draw(maplepointslabel_pos);
		}

		// Check if tab exists in slotrange
		if (slotrange.find(tab) == slotrange.end()) {
			return;
		}
		auto& range = slotrange.at(tab);

		size_t numslots = inventory.get_slotmax(tab);
		size_t firstslot = full_enabled ? 1 : range.first;
		size_t lastslot = full_enabled ? max_full_slots : range.second;

		// Fix: Start from slot 1, not 0, to match load_icons
		for (size_t i = 1; i <= max_full_slots; i++)
		{
			Point<int16_t> slotpos = get_slotpos(i);

			if (icons.find(i) != icons.end())
			{
				auto& icon = icons.at(i);

				if (icon && i >= firstslot && i <= lastslot)
					icon->draw(position + slotpos);
			}
			else
			{
				if (i > numslots && i <= lastslot && disabled.is_valid())
					disabled.draw(position + slotpos);
			}

#if LOG_LEVEL >= LOG_UI
			if (i <= lastslot && i < max_full_slots)
				slot_labels[i].draw(position + get_slotpos(i + 1) - Point<int16_t>(0, 5));
#endif
		}

		int16_t bulletslot = inventory.get_bulletslot();

		if (tab == InventoryType::Id::USE && is_visible(bulletslot) && projectile.is_valid())
			projectile.draw(position + get_slotpos(bulletslot));

		UIElement::draw_buttons(alpha);

		if (newtab != InventoryType::Id::NONE)
		{
			if (newtab == tab)
			{
				newitemtaben.draw(position + get_tabpos(newtab) - Point<int16_t>(2, 3), alpha);

				if (is_visible(newslot))
					newitemslot.draw(position + get_slotpos(newslot) + Point<int16_t>(1, 1), alpha);
			}
			else
			{
				newitemtabdis.draw(position + get_tabpos(newtab) - Point<int16_t>(2, 1), alpha);
			}
		}
	}

	void UIItemInventory::update()
	{
		// Handle deferred initialization to avoid UI toggle deadlock
		if (needs_init) {
			needs_init = false;
			set_full(false);  // Safe to call now, outside constructor/toggle context
		}
		
		UIElement::update();

		newitemtaben.update(6);
		newitemtabdis.update(6);
		newitemslot.update(6);

		std::string meso_str = std::to_string(inventory.get_meso());
		string_format::split_number(meso_str);

		mesolabel.change_text(meso_str);
	}

	void UIItemInventory::update_slot(int16_t slot)
	{
		// Fix: Ignore slot 0 to prevent freezes
		if (slot <= 0) {
			return;
		}
		
		if (int32_t item_id = inventory.get_item_id(tab, slot))
		{
			int16_t count;

			if (tab == InventoryType::Id::EQUIP)
				count = -1;
			else
				count = inventory.get_item_count(tab, slot);

			const bool untradable = ItemData::get(item_id).is_untradable();
			const bool cashitem = ItemData::get(item_id).is_cashitem();
			const Texture& texture = ItemData::get(item_id).get_icon(false);
			EquipSlot::Id eqslot = inventory.find_equipslot(item_id);

			icons[slot] = std::make_unique<Icon>(
				std::make_unique<ItemIcon>(*this, tab, eqslot, slot, item_id, count, untradable, cashitem),
				texture, count
			);
		}
		else if (icons.count(slot))
		{
			icons.erase(slot);
		}
	}

	void UIItemInventory::load_icons()
	{
		icons.clear();

		uint8_t numslots = inventory.get_slotmax(tab);

		// Fix: Start at slot 1, not 0 (slot 0 causes freezes)
		for (size_t i = 1; i <= max_full_slots; i++) {
			if (i <= numslots) {
				update_slot(static_cast<int16_t>(i));
			}
		}
	}

	Button::State UIItemInventory::button_pressed(uint16_t buttonid)
	{
		InventoryType::Id oldtab = tab;

		switch (buttonid)
		{
			case Buttons::BT_CLOSE:
			{
				toggle_active();

				return Button::State::NORMAL;
			}
			case Buttons::BT_TAB_EQUIP:
			{
				tab = InventoryType::Id::EQUIP;
				break;
			}
			case Buttons::BT_TAB_USE:
			{
				tab = InventoryType::Id::USE;
				break;
			}
			case Buttons::BT_TAB_SETUP:
			{
				tab = InventoryType::Id::SETUP;
				break;
			}
			case Buttons::BT_TAB_ETC:
			{
				tab = InventoryType::Id::ETC;
				break;
			}
			case Buttons::BT_TAB_CASH:
			{
				tab = InventoryType::Id::CASH;
				break;
			}
			case Buttons::BT_TAB_DEC:
			{
				tab = InventoryType::Id::DEC;
				break;
			}
			case Buttons::BT_GATHER:
			case Buttons::BT_GATHER_SM:
			{
				GatherItemsPacket(tab).dispatch();
				break;
			}
			case Buttons::BT_SORT:
			case Buttons::BT_SORT_SM:
			{
				SortItemsPacket(tab).dispatch();
				break;
			}
			case Buttons::BT_FULL:
			{
				set_full(true);

				return Button::State::NORMAL;
			}
			case Buttons::BT_SMALL:
			{
				set_full(false);

				return Button::State::NORMAL;
			}
			case Buttons::BT_COIN:
			case Buttons::BT_COIN_SM:
			case Buttons::BT_POINT:
			case Buttons::BT_POINT_SM:
			case Buttons::BT_UPGRADE:
			case Buttons::BT_UPGRADE_SM:
			case Buttons::BT_APPRAISE:
			case Buttons::BT_APPRAISE_SM:
			case Buttons::BT_EXTRACT:
			case Buttons::BT_EXTRACT_SM:
			case Buttons::BT_DISASSEMBLE:
			case Buttons::BT_DISASSEMBLE_SM:
			case Buttons::BT_TOAD:
			case Buttons::BT_TOAD_SM:
			case Buttons::BT_CASHSHOP:
			{
				return Button::State::NORMAL;
			}
		}

		if (tab != oldtab)
		{
			uint16_t row = slotrange.at(tab).first / slot_col;
			slider.setrows(row, 6, inventory.get_slotmax(tab) / slot_col);

			// V87 doesn't have tab buttons
			uint16_t oldtab_btn = button_by_tab(oldtab);
			uint16_t newtab_btn = button_by_tab(tab);
			
			if (oldtab_btn < buttons.size() && buttons[oldtab_btn])
				buttons[oldtab_btn]->set_state(Button::State::NORMAL);
			if (newtab_btn < buttons.size() && buttons[newtab_btn])
				buttons[newtab_btn]->set_state(Button::State::PRESSED);

			load_icons();
			set_sort(false);
		}

		return Button::State::IDENTITY;
	}

	void UIItemInventory::doubleclick(Point<int16_t> cursorpos)
	{
		int16_t slot = slot_by_position(cursorpos - position);

		if (icons.count(slot) && is_visible(slot))
		{
			if (int32_t item_id = inventory.get_item_id(tab, slot))
			{
				switch (tab)
				{
					case InventoryType::Id::EQUIP:
					{
						if (can_wear_equip(slot))
						{
							EquipSlot::Id equipslot = inventory.find_equipslot(item_id);

							if (equipslot == EquipSlot::Id::NONE)
							{
								LOG(LOG_DEBUG, "Could not find appropriate EquipSlot::Id for item [" << item_id << "]. Equip would be dropped.");
								break;
							}

							EquipItemPacket(slot, equipslot).dispatch();
						}

						break;
					}
					case InventoryType::Id::USE:
					{
						UseItemPacket(slot, item_id).dispatch();
						break;
					}
				}
			}
		}
	}

	bool UIItemInventory::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		int16_t slot = slot_by_position(cursorpos - position);

		if (slot > 0)
		{
			int32_t item_id = inventory.get_item_id(tab, slot);
			EquipSlot::Id eqslot;
			bool equip;

			if (item_id && tab == InventoryType::Id::EQUIP)
			{
				eqslot = inventory.find_equipslot(item_id);
				equip = true;
			}
			else
			{
				eqslot = EquipSlot::Id::NONE;
				equip = false;
			}

			ignore_tooltip = true;

			return icon.drop_on_items(tab, eqslot, slot, equip);
		}

		return true;
	}

	Cursor::State UIItemInventory::send_cursor(bool pressed, Point<int16_t> cursorpos)
	{
		// For v87, we need to handle null buttons
		// First, handle button interactions manually to avoid null pointer access
		Cursor::State ret = pressed ? Cursor::State::CLICKING : Cursor::State::IDLE;
		
		for (auto& btit : buttons)
		{
			// Check if button exists before accessing it
			if (btit.second && btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();
					btit.second->set_state(Button::State::MOUSEOVER);
					ret = Cursor::State::CANCLICK;
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (pressed)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(Button::State::PRESSED);
						button_pressed(btit.first);
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
				else if (!pressed && btit.second->get_state() == Button::State::PRESSED)
				{
					btit.second->set_state(Button::State::MOUSEOVER);
					ret = Cursor::State::CANCLICK;
				}
			}
			else if (btit.second && btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}
		
		// Now handle drag state
		// Check if cursor is in drag range (title bar area)
		auto bounds = Rectangle<int16_t>(position, position + dragarea);
		if (pressed && bounds.contains(cursorpos))
		{
			dragged = true;
		}
		
		// If we're already processing a button, return that state
		if (ret != Cursor::State::IDLE && ret != Cursor::State::CLICKING)
		{
			return ret;
		}
		
		// Handle drag state manually to avoid base class button access
		if (dragged)
		{
			clear_tooltip();

			return Cursor::State::CLICKING;
		}

		Point<int16_t> cursor_relative = cursorpos - position;

		if (!full_enabled && slider.isenabled())
		{
			Cursor::State sstate = slider.send_cursor(cursor_relative, pressed);

			if (sstate != Cursor::State::IDLE)
			{
				clear_tooltip();

				return sstate;
			}
		}

		int16_t slot = slot_by_position(cursor_relative);
		Icon* icon = get_icon(slot);
		bool is_icon = icon && is_visible(slot);

		if (is_icon)
		{
			if (pressed)
			{
				Point<int16_t> slotpos = get_slotpos(slot);
				icon->start_drag(cursor_relative - slotpos);

				UI::get().drag_icon(icon);

				clear_tooltip();

				return Cursor::State::GRABBING;
			}
			else if (!ignore_tooltip)
			{
				show_item(slot);

				return Cursor::State::CANGRAB;
			}
			else
			{
				ignore_tooltip = false;

				return Cursor::State::CANGRAB;
			}
		}
		else
		{
			clear_tooltip();

			// Return the state we calculated with null checks instead of calling base class
			return ret;
		}
	}

	void UIItemInventory::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				toggle_active();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				clear_tooltip();

				InventoryType::Id newtab;

				switch (tab)
				{
					case InventoryType::Id::EQUIP:
						newtab = InventoryType::Id::USE;
						break;
					case InventoryType::Id::USE:
						newtab = InventoryType::Id::ETC;
						break;
					case InventoryType::Id::ETC:
						newtab = InventoryType::Id::SETUP;
						break;
					case InventoryType::Id::SETUP:
						newtab = InventoryType::Id::CASH;
						break;
					case InventoryType::Id::CASH:
						newtab = InventoryType::Id::DEC;
						break;
					case InventoryType::Id::DEC:
						newtab = InventoryType::Id::EQUIP;
						break;
				}

				button_pressed(button_by_tab(newtab));
			}
		}
	}

	UIElement::Type UIItemInventory::get_type() const
	{
		return TYPE;
	}

	void UIItemInventory::modify(InventoryType::Id type, int16_t slot, int8_t mode, int16_t arg)
	{
		if (slot <= 0)
			return;

		if (type == tab)
		{
			switch (mode)
			{
				case Inventory::Modification::ADD:
				{
					update_slot(slot);

					newtab = type;
					newslot = slot;
					break;
				}
				case Inventory::Modification::CHANGECOUNT:
				case Inventory::Modification::ADDCOUNT:
				{
					if (auto icon = get_icon(slot))
						icon->set_count(arg);

					break;
				}
				case Inventory::Modification::SWAP:
				{
					if (arg != slot)
					{
						update_slot(slot);
						update_slot(arg);
					}

					break;
				}
				case Inventory::Modification::REMOVE:
				{
					update_slot(slot);
					break;
				}
			}
		}

		switch (mode)
		{
			case Inventory::Modification::ADD:
			case Inventory::Modification::ADDCOUNT:
			{
				newtab = type;
				newslot = slot;
				break;
			}
			case Inventory::Modification::CHANGECOUNT:
			case Inventory::Modification::SWAP:
			case Inventory::Modification::REMOVE:
			{
				if (newslot == slot && newtab == type)
					clear_new();

				break;
			}
		}
	}

	void UIItemInventory::set_sort(bool enabled)
	{
		sort_enabled = enabled;

		// V87 doesn't have sort/gather buttons, so check if they exist
		if (full_enabled)
		{
			if (sort_enabled)
			{
				if (buttons[Buttons::BT_SORT]) buttons[Buttons::BT_SORT]->set_active(false);
				if (buttons[Buttons::BT_SORT_SM]) buttons[Buttons::BT_SORT_SM]->set_active(true);
				if (buttons[Buttons::BT_GATHER]) buttons[Buttons::BT_GATHER]->set_active(false);
				if (buttons[Buttons::BT_GATHER_SM]) buttons[Buttons::BT_GATHER_SM]->set_active(false);
			}
			else
			{
				if (buttons[Buttons::BT_SORT]) buttons[Buttons::BT_SORT]->set_active(false);
				if (buttons[Buttons::BT_SORT_SM]) buttons[Buttons::BT_SORT_SM]->set_active(false);
				if (buttons[Buttons::BT_GATHER]) buttons[Buttons::BT_GATHER]->set_active(false);
				if (buttons[Buttons::BT_GATHER_SM]) buttons[Buttons::BT_GATHER_SM]->set_active(true);
			}
		}
		else
		{
			if (sort_enabled)
			{
				if (buttons[Buttons::BT_SORT]) buttons[Buttons::BT_SORT]->set_active(true);
				if (buttons[Buttons::BT_SORT_SM]) buttons[Buttons::BT_SORT_SM]->set_active(false);
				if (buttons[Buttons::BT_GATHER]) buttons[Buttons::BT_GATHER]->set_active(false);
				if (buttons[Buttons::BT_GATHER_SM]) buttons[Buttons::BT_GATHER_SM]->set_active(false);
			}
			else
			{
				if (buttons[Buttons::BT_SORT]) buttons[Buttons::BT_SORT]->set_active(false);
				if (buttons[Buttons::BT_SORT_SM]) buttons[Buttons::BT_SORT_SM]->set_active(false);
				if (buttons[Buttons::BT_GATHER]) buttons[Buttons::BT_GATHER]->set_active(true);
				if (buttons[Buttons::BT_GATHER_SM]) buttons[Buttons::BT_GATHER_SM]->set_active(false);
			}
		}
	}

	void UIItemInventory::change_tab(InventoryType::Id type)
	{
		button_pressed(button_by_tab(type));
	}

	void UIItemInventory::clear_new()
	{
		newtab = InventoryType::Id::NONE;
		newslot = 0;
	}

	void UIItemInventory::toggle_active()
	{
		LOG(LOG_DEBUG, "[UIItemInventory] toggle_active() START, current active=" << active);
		UIElement::toggle_active();
		LOG(LOG_DEBUG, "[UIItemInventory] toggle_active() after UIElement::toggle_active(), active=" << active);

		if (!active)
		{
			LOG(LOG_DEBUG, "[UIItemInventory] Inventory closing - clearing new items and tooltips");
			clear_new();
			clear_tooltip();
		}
		else
		{
			LOG(LOG_DEBUG, "[UIItemInventory] Inventory opening - clearing tooltips");
			// Clear any lingering tooltips when opening to prevent cross-UI conflicts
			clear_tooltip();
		}
		LOG(LOG_DEBUG, "[UIItemInventory] toggle_active() END");
	}

	void UIItemInventory::remove_cursor()
	{
		UIDragElement::remove_cursor();

		slider.remove_cursor();
	}

	void UIItemInventory::show_item(int16_t slot)
	{
		if (tab == InventoryType::Id::EQUIP)
		{
			UI::get().show_equip(Tooltip::Parent::ITEMINVENTORY, slot);
		}
		else
		{
			int32_t item_id = inventory.get_item_id(tab, slot);
			UI::get().show_item(Tooltip::Parent::ITEMINVENTORY, item_id);
		}
	}

	void UIItemInventory::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::ITEMINVENTORY);
	}

	bool UIItemInventory::is_visible(int16_t slot) const
	{
		return !is_not_visible(slot);
	}

	bool UIItemInventory::is_not_visible(int16_t slot) const
	{
		auto& range = slotrange.at(tab);

		if (full_enabled)
			return slot < 1 || slot > max_full_slots;
		else
			return slot < range.first || slot > range.second;
	}

	bool UIItemInventory::can_wear_equip(int16_t slot) const
	{
		const Player& player = Stage::get().get_player();
		const CharStats& stats = player.get_stats();
		const CharLook& look = player.get_look();
		const bool alerted = look.get_alerted();

		if (alerted)
		{
			UI::get().emplace<UIOk>("You cannot complete this action right now.\\nEvade the attack and try again.", [](bool) {});
			return false;
		}

		const int32_t item_id = inventory.get_item_id(InventoryType::Id::EQUIP, slot);
		const EquipData& equipdata = EquipData::get(item_id);
		const ItemData& itemdata = equipdata.get_itemdata();

		const int8_t reqGender = itemdata.get_gender();
		const bool female = stats.get_female();

		switch (reqGender)
		{
			// Male
			case 0:
			{
				if (female)
					return false;

				break;
			}
			// Female
			case 1:
			{
				if (!female)
					return false;

				break;
			}
			// Unisex
			case 2:
			default:
			{
				break;
			}
		}

		const std::string jobname = stats.get_jobname();

		if (jobname == "GM" || jobname == "SuperGM")
			return true;

		// TODO: Remove from EquipTooltip and move into Job?
		bool can_wear = false;

		uint16_t job = stats.get_stat(MapleStat::Id::JOB) / 100;
		int16_t reqJOB = equipdata.get_reqstat(MapleStat::Id::JOB);

		switch (reqJOB)
		{
			case 0: // Common
			{
				can_wear = true;
				break;
			}
			case 1: // Warrior
			{
				if (job == 1 || job >= 20)
					can_wear = true;

				break;
			}
			case 2: // Magician
			{
				if (job == 2)
					can_wear = true;

				break;
			}
			case 3: // Magician, Warrior
			{
				if (job == 1 || job >= 20 || job == 2)
					can_wear = true;

				break;
			}
			case 4: // Bowman
			{
				if (job == 3)
					can_wear = true;

				break;
			}
			case 8: // Thief
			{
				if (job == 4)
					can_wear = true;

				break;
			}
			case 16: // Pirate
			{
				if (job == 5)
					can_wear = true;

				break;
			}
			default:
			{
				can_wear = false;
			}
		}

		if (!can_wear)
		{
			UI::get().emplace<UIOk>("Your current job\\ncannot equip the selected item.", [](bool) {});
			return false;
		}
		// End of TODO

		int16_t reqLevel = equipdata.get_reqstat(MapleStat::Id::LEVEL);
		int16_t reqDEX = equipdata.get_reqstat(MapleStat::Id::DEX);
		int16_t reqSTR = equipdata.get_reqstat(MapleStat::Id::STR);
		int16_t reqLUK = equipdata.get_reqstat(MapleStat::Id::LUK);
		int16_t reqINT = equipdata.get_reqstat(MapleStat::Id::INT);
		int16_t reqFAME = equipdata.get_reqstat(MapleStat::Id::FAME);

		int8_t i = 0;

		if (reqLevel > stats.get_stat(MapleStat::Id::LEVEL))
			i++;
		else if (reqDEX > stats.get_total(EquipStat::Id::DEX))
			i++;
		else if (reqSTR > stats.get_total(EquipStat::Id::STR))
			i++;
		else if (reqLUK > stats.get_total(EquipStat::Id::LUK))
			i++;
		else if (reqINT > stats.get_total(EquipStat::Id::INT))
			i++;
		else if (reqFAME > stats.get_honor())
			i++;

		if (i > 0)
		{
			UI::get().emplace<UIOk>("Your stats are too low to equip this item\\nor you do not meet the job requirement.", [](bool) {});
			return false;
		}

		return true;
	}

	int16_t UIItemInventory::slot_by_position(Point<int16_t> cursorpos) const
	{
		Point<int16_t> cursor_offset = cursorpos - slot_pos;

		int16_t xoff = cursor_offset.x();
		int16_t yoff = cursor_offset.y();

		int16_t cur_x = cursorpos.x();
		int16_t slot_x = slot_pos.x();
		int16_t xmin = slot_x;
		int16_t xmax = (icon_width + slot_space_x) * (full_enabled ? slot_col * 4 : slot_col) - (full_enabled ? slot_space_x : 0);

		int16_t cur_y = cursorpos.y();
		int16_t slot_y = slot_pos.y();
		int16_t ymin = slot_y;
		int16_t ymax = (icon_height + slot_space_y) * (full_enabled ? slot_row + 1 : slot_row - 1) - (full_enabled ? slot_space_y : 0);

		int16_t slot = 0;
		int16_t absslot = full_enabled ? 1 : slotrange.at(tab).first;

		int16_t col = cur_x / (icon_width + slot_space_x);
		int16_t row = cur_y / (icon_height + slot_space_y) - 1;

		div_t div = std::div(col, 4);
		slot = col + absslot + (4 * row) + (div.quot * 28);

		if (cur_x < xmin || cur_x > xmax || cur_y < ymin || cur_y > ymax)
			slot = 0;

		LOG(LOG_UI,
			"Slot: " << slot << " Col: " << col << " Row: " << row << " "
			<< cur_x << " < (" << xmin << ") || "
			<< cur_x << " > (" << xmax << ") || "
			<< cur_y << " < (" << ymin << ") && "
			<< cur_y << " > (" << ymax << ")");

		if (is_visible(slot))
			return slot;

		return 0;
	}

	Point<int16_t> UIItemInventory::get_slotpos(int16_t slot) const
	{
		// Add safety check for tab
		if (slotrange.find(tab) == slotrange.end()) {
			LOG(LOG_ERROR, "[UIItemInventory] get_slotpos() ERROR: tab " << (int)tab << " not found in slotrange!");
			return Point<int16_t>(0, 0);
		}
		int16_t absslot = slot - (full_enabled ? 1 : slotrange.at(tab).first);

		div_t div4 = std::div(absslot, 4);
		div_t div32 = std::div(absslot, 32);

		int16_t row = div4.quot - (8 * div32.quot);
		int16_t col = div4.rem + (4 * div32.quot);

		return slot_pos + Point<int16_t>((col * 10) + (col * 32), (row * 10) + (row * 32));
	}

	Point<int16_t> UIItemInventory::get_tabpos(InventoryType::Id tb) const
	{
		int8_t fixed_tab = tb;

		switch (tb)
		{
			case InventoryType::Id::ETC:
				fixed_tab = 3;
				break;
			case InventoryType::Id::SETUP:
				fixed_tab = 4;
				break;
		}

		return Point<int16_t>(10 + ((fixed_tab - 1) * 31), 29);
	}

	uint16_t UIItemInventory::button_by_tab(InventoryType::Id tb) const
	{
		switch (tb)
		{
			case InventoryType::Id::EQUIP:
				return Buttons::BT_TAB_EQUIP;
			case InventoryType::Id::USE:
				return Buttons::BT_TAB_USE;
			case InventoryType::Id::SETUP:
				return Buttons::BT_TAB_SETUP;
			case InventoryType::Id::ETC:
				return Buttons::BT_TAB_ETC;
			case InventoryType::Id::CASH:
				return Buttons::BT_TAB_CASH;
			default:
				return Buttons::BT_TAB_DEC;
		}
	}

	Icon* UIItemInventory::get_icon(int16_t slot)
	{
		auto iter = icons.find(slot);

		if (iter != icons.end())
			return iter->second.get();
		else
			return nullptr;
	}

	void UIItemInventory::set_full(bool enabled)
	{
		LOG(LOG_DEBUG, "[UIItemInventory] set_full(" << enabled << ") START");
		full_enabled = enabled;

		if (full_enabled)
		{
			dimension = bg_full_dimensions;

			// Safe button access with null checks
			if (buttons[Buttons::BT_FULL]) buttons[Buttons::BT_FULL]->set_active(false);
			if (buttons[Buttons::BT_SMALL]) buttons[Buttons::BT_SMALL]->set_active(true);
		}
		else
		{
			dimension = bg_dimensions;

			if (buttons[Buttons::BT_FULL]) buttons[Buttons::BT_FULL]->set_active(true);
			if (buttons[Buttons::BT_SMALL]) buttons[Buttons::BT_SMALL]->set_active(false);
		}

		dragarea = Point<int16_t>(dimension.x(), 20);

		if (buttons[Buttons::BT_CLOSE]) 
			buttons[Buttons::BT_CLOSE]->set_position(Point<int16_t>(dimension.x() - 20, 6));

		// V87 doesn't have these buttons, so check if they exist
		if (buttons[Buttons::BT_COIN]) buttons[Buttons::BT_COIN]->set_active(!enabled);
		if (buttons[Buttons::BT_POINT]) buttons[Buttons::BT_POINT]->set_active(!enabled);
		if (buttons[Buttons::BT_UPGRADE]) buttons[Buttons::BT_UPGRADE]->set_active(!enabled);
		if (buttons[Buttons::BT_APPRAISE]) buttons[Buttons::BT_APPRAISE]->set_active(!enabled);
		if (buttons[Buttons::BT_EXTRACT]) buttons[Buttons::BT_EXTRACT]->set_active(!enabled);
		if (buttons[Buttons::BT_DISASSEMBLE]) buttons[Buttons::BT_DISASSEMBLE]->set_active(!enabled);
		if (buttons[Buttons::BT_TOAD]) buttons[Buttons::BT_TOAD]->set_active(!enabled);
		if (buttons[Buttons::BT_CASHSHOP]) buttons[Buttons::BT_CASHSHOP]->set_active(!enabled);

		if (buttons[Buttons::BT_COIN_SM]) buttons[Buttons::BT_COIN_SM]->set_active(enabled);
		if (buttons[Buttons::BT_POINT_SM]) buttons[Buttons::BT_POINT_SM]->set_active(enabled);
		if (buttons[Buttons::BT_UPGRADE_SM]) buttons[Buttons::BT_UPGRADE_SM]->set_active(enabled);
		if (buttons[Buttons::BT_APPRAISE_SM]) buttons[Buttons::BT_APPRAISE_SM]->set_active(enabled);
		if (buttons[Buttons::BT_EXTRACT_SM]) buttons[Buttons::BT_EXTRACT_SM]->set_active(enabled);
		if (buttons[Buttons::BT_DISASSEMBLE_SM]) buttons[Buttons::BT_DISASSEMBLE_SM]->set_active(enabled);
		if (buttons[Buttons::BT_TOAD_SM]) buttons[Buttons::BT_TOAD_SM]->set_active(enabled);
		if (buttons[Buttons::BT_CASHSHOP]) buttons[Buttons::BT_CASHSHOP]->set_active(enabled);

		set_sort(sort_enabled);
		LOG(LOG_DEBUG, "[UIItemInventory] set_full() calling load_icons()");
		load_icons();
		LOG(LOG_DEBUG, "[UIItemInventory] set_full() END");
	}

	void UIItemInventory::ItemIcon::set_count(int16_t c)
	{
		count = c;
	}

	Icon::IconType UIItemInventory::ItemIcon::get_type()
	{
		return Icon::IconType::ITEM;
	}

	UIItemInventory::ItemIcon::ItemIcon(const UIItemInventory& parent, InventoryType::Id st, EquipSlot::Id eqs, int16_t s, int32_t iid, int16_t c, bool u, bool cash) : parent(parent)
	{
		sourcetab = st;
		eqsource = eqs;
		source = s;
		item_id = iid;
		count = c;
		untradable = u;
		cashitem = cash;
	}

	void UIItemInventory::ItemIcon::drop_on_stage() const
	{
		constexpr const char* dropmessage = "How many will you drop?";
		constexpr const char* untradablemessage = "This item can't be taken back once thrown away.\\nWill you still drop it?";
		constexpr const char* cashmessage = "You can't drop this item.";

		if (cashitem)
		{
			UI::get().emplace<UIOk>(cashmessage, [](bool) {});
		}
		else
		{
			if (untradable)
			{
				auto onok = [&, dropmessage](bool ok)
				{
					if (ok)
					{
						if (count <= 1)
						{
							MoveItemPacket(sourcetab, source, 0, 1).dispatch();
						}
						else
						{
							auto onenter = [&](int32_t qty)
							{
								MoveItemPacket(sourcetab, source, 0, qty).dispatch();
							};

							UI::get().emplace<UIEnterNumber>(dropmessage, onenter, count, count);
						}
					}
				};

				UI::get().emplace<UIYesNo>(untradablemessage, onok);
			}
			else
			{
				if (count <= 1)
				{
					MoveItemPacket(sourcetab, source, 0, 1).dispatch();
				}
				else
				{
					auto onenter = [&](int32_t qty)
					{
						MoveItemPacket(sourcetab, source, 0, qty).dispatch();
					};

					UI::get().emplace<UIEnterNumber>(dropmessage, onenter, count, count);
				}
			}
		}
	}

	void UIItemInventory::ItemIcon::drop_on_equips(EquipSlot::Id eqslot) const
	{
		switch (sourcetab)
		{
			case InventoryType::Id::EQUIP:
			{
				if (eqsource == eqslot)
					if (parent.can_wear_equip(source))
						EquipItemPacket(source, eqslot).dispatch();

				Sound(Sound::Name::DRAGEND).play();
				break;
			}
			case InventoryType::Id::USE:
			{
				ScrollEquipPacket(source, eqslot).dispatch();
				break;
			}
		}
	}

	bool UIItemInventory::ItemIcon::drop_on_items(InventoryType::Id tab, EquipSlot::Id, int16_t slot, bool) const
	{
		if (tab != sourcetab || slot == source)
			return true;

		MoveItemPacket(tab, source, slot, 1).dispatch();

		return true;
	}

	void UIItemInventory::ItemIcon::drop_on_bindings(Point<int16_t> cursorposition, bool remove) const
	{
		if (sourcetab == InventoryType::Id::USE || sourcetab == InventoryType::Id::SETUP)
		{
			auto keyconfig = UI::get().get_element<UIKeyConfig>();
			Keyboard::Mapping mapping = Keyboard::Mapping(KeyType::ITEM, item_id);

			if (remove)
				keyconfig->unstage_mapping(mapping);
			else
				keyconfig->stage_mapping(cursorposition, mapping);
		}
	}
}