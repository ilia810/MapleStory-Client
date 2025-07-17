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
#include "UIEquipInventory.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../UITypes/UIItemInventory.h"

#include "../../Audio/Audio.h"
#include "../../Data/ItemData.h"

#include "../../Net/Packets/InventoryPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIEquipInventory::UIEquipInventory(const Inventory& invent) : UIDragElement<PosEQINV>(), inventory(invent), tab(Buttons::BT_TAB1), hasPendantSlot(false), hasPocketSlot(false)
	{
		// Column 1
		iconpositions[EquipSlot::Id::RING1] = Point<int16_t>(14, 50);
		iconpositions[EquipSlot::Id::RING2] = Point<int16_t>(14, 91);
		iconpositions[EquipSlot::Id::RING3] = Point<int16_t>(14, 132);
		iconpositions[EquipSlot::Id::RING4] = Point<int16_t>(14, 173);
		iconpositions[EquipSlot::Id::POCKET] = Point<int16_t>(14, 214);
		iconpositions[EquipSlot::Id::BOOK] = Point<int16_t>(14, 255);

		// Column 2
		//iconpositions[EquipSlot::Id::NONE] = Point<int16_t>(55, 50);
		iconpositions[EquipSlot::Id::PENDANT2] = Point<int16_t>(55, 91);
		iconpositions[EquipSlot::Id::PENDANT1] = Point<int16_t>(55, 132);
		iconpositions[EquipSlot::Id::WEAPON] = Point<int16_t>(55, 173);
		iconpositions[EquipSlot::Id::BELT] = Point<int16_t>(55, 214);
		//iconpositions[EquipSlot::Id::NONE] = Point<int16_t>(55, 255);

		// Column 3
		iconpositions[EquipSlot::Id::HAT] = Point<int16_t>(96, 50);
		iconpositions[EquipSlot::Id::FACE] = Point<int16_t>(96, 91);
		iconpositions[EquipSlot::Id::EYEACC] = Point<int16_t>(96, 132);
		iconpositions[EquipSlot::Id::TOP] = Point<int16_t>(96, 173);
		iconpositions[EquipSlot::Id::BOTTOM] = Point<int16_t>(96, 214);
		iconpositions[EquipSlot::Id::SHOES] = Point<int16_t>(96, 255);

		// Column 4
		//iconpositions[EquipSlot::Id::NONE] = Point<int16_t>(137, 50);
		//iconpositions[EquipSlot::Id::NONE] = Point<int16_t>(137, 91);
		iconpositions[EquipSlot::Id::EARACC] = Point<int16_t>(137, 132);
		iconpositions[EquipSlot::Id::SHOULDER] = Point<int16_t>(137, 173);
		iconpositions[EquipSlot::Id::GLOVES] = Point<int16_t>(137, 214);
		iconpositions[EquipSlot::Id::ANDROID] = Point<int16_t>(137, 255);

		// Column 5
		iconpositions[EquipSlot::Id::EMBLEM] = Point<int16_t>(178, 50);
		iconpositions[EquipSlot::Id::BADGE] = Point<int16_t>(178, 91);
		iconpositions[EquipSlot::Id::MEDAL] = Point<int16_t>(178, 132);
		iconpositions[EquipSlot::Id::SUBWEAPON] = Point<int16_t>(178, 173);
		iconpositions[EquipSlot::Id::CAPE] = Point<int16_t>(178, 214);
		iconpositions[EquipSlot::Id::HEART] = Point<int16_t>(178, 255);

		//iconpositions[EquipSlot::Id::SHIELD] = Point<int16_t>(142, 124);
		//iconpositions[EquipSlot::Id::TAMEDMOB] = Point<int16_t>(142, 91);
		//iconpositions[EquipSlot::Id::SADDLE] = Point<int16_t>(76, 124);

		tab_source[Buttons::BT_TAB0] = "Equip";
		tab_source[Buttons::BT_TAB1] = "Cash";
		tab_source[Buttons::BT_TAB2] = "Pet";
		tab_source[Buttons::BT_TAB3] = "Android";

		nl::node close = nl::nx::UI["Basic.img"]["BtClose3"];
		
		// Use simplified approach - only UI.UIWindow.backgrnd
		nl::node main_bg = nl::nx::UI["UIWindow.img"]["backgrnd"];
		
		// Use the same background for all tabs
		background[Buttons::BT_TAB0] = main_bg;
		background[Buttons::BT_TAB1] = main_bg;
		background[Buttons::BT_TAB2] = main_bg;
		background[Buttons::BT_TAB3] = main_bg;
		
		// Still need to load the Equip node for other UI elements
		nl::node Equip = nl::nx::UI["UIWindow.img"]["Equip"];
		nl::node EquipGL = Equip; // Fallback to same node
		bool is_legacy = true; // Simplified mode

		// Load slots if available
		for (uint16_t i = Buttons::BT_TAB0; i < Buttons::BT_TABE; i++) {
			nl::node slots_node = Equip[tab_source[i]]["Slots"];
			if (!slots_node.name().empty()) {
				for (auto slot : slots_node) {
					if (slot.name().find("_") == std::string::npos) {
						Slots[i].emplace_back(slot);
					}
				}
			}
		}

		// Use the already loaded main_bg for dimensions
		Point<int16_t> bg_dimensions = Texture(main_bg).get_dimensions();
		totem_dimensions = bg_dimensions; // Use same dimensions
		totem_adj = Point<int16_t>(0, 0); // No adjustment needed

		// Don't add any background sprites - the background[tab] will handle all drawing

		// Legacy: May not have tabbar, use fallback or skip
		nl::node tabbar_node = Equip["tabbar"];
		if (!tabbar_node.name().empty()) {
			tabbar = tabbar_node;
		}
		
		// Load disabled states from Equip node
		nl::node disabled_node = Equip["disabled"];
		nl::node disabled2_node = Equip["disabled2"];
		
		if (!disabled_node.name().empty()) {
			disabled = disabled_node;
		} else {
			// Fallback: use main background as placeholder
			disabled = main_bg;
		}
		
		if (!disabled2_node.name().empty()) {
			disabled2 = disabled2_node;
		} else {
			// Fallback: use disabled if it exists, otherwise use background
			disabled2 = !disabled_node.name().empty() ? disabled_node : main_bg;
		}

		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(bg_dimensions.x() - 19, 5));
		
		// Simplified button loading - use available buttons from Equip node
		nl::node btSlot = Equip["BtDetail"];
		if (btSlot.name().empty()) {
			btSlot = Equip["BtSlot"];
			if (btSlot.name().empty()) {
				btSlot = close; // Fallback to close button style
			}
		}
		buttons[Buttons::BT_SLOT] = std::make_unique<MapleButton>(btSlot);
		
		// These buttons may not exist in older versions, use fallbacks
		buttons[Buttons::BT_EFFECT] = std::make_unique<MapleButton>(close);
		buttons[Buttons::BT_SALON] = std::make_unique<MapleButton>(close);
		
		// Pet-related buttons
		nl::node btConsume = Equip["BtPet1"];
		if (btConsume.name().empty()) btConsume = close;
		buttons[Buttons::BT_CONSUMESETTING] = std::make_unique<MapleButton>(btConsume);
		
		nl::node btException = Equip["BtPet2"];
		if (btException.name().empty()) btException = close;
		buttons[Buttons::BT_EXCEPTION] = std::make_unique<MapleButton>(btException);
		
		nl::node btShop = Equip["BtCashshop"];
		if (btShop.name().empty()) btShop = close;
		buttons[Buttons::BT_SHOP] = std::make_unique<MapleButton>(btShop);

		buttons[Buttons::BT_CONSUMESETTING]->set_state(Button::State::DISABLED);
		buttons[Buttons::BT_EXCEPTION]->set_state(Button::State::DISABLED);
		buttons[Buttons::BT_SHOP]->set_state(Button::State::DISABLED);

		// Simplified tab creation
		nl::node Tab = Equip["Tab"];
		if (!Tab.name().empty()) {
			// Use Tab structure if available
			for (uint16_t i = Buttons::BT_TAB0; i < Buttons::BT_TABE; i++) {
				nl::node disabled = Tab["disabled"][i];
				nl::node enabled = Tab["enabled"][i];
				if (!disabled.name().empty() && !enabled.name().empty()) {
					buttons[Buttons::BT_TAB0 + i] = std::make_unique<TwoSpriteButton>(disabled, enabled, Point<int16_t>(0, 3));
				} else {
					// Fallback if tab sprites are missing
					buttons[Buttons::BT_TAB0 + i] = std::make_unique<MapleButton>(close, Point<int16_t>(i * 30, 3));
				}
			}
		} else {
			// No Tab structure, use fallback
			for (uint16_t i = Buttons::BT_TAB0; i < Buttons::BT_TABE; i++) {
				buttons[Buttons::BT_TAB0 + i] = std::make_unique<MapleButton>(close, Point<int16_t>(i * 30, 3));
			}
		}

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(bg_dimensions.x(), 20);

		load_icons();
		change_tab(Buttons::BT_TAB0);
	}

	void UIEquipInventory::draw(float alpha) const
	{
		UIElement::draw(alpha);

		// Only draw the current tab's background, not all of them
		background[tab].draw(position);
		
		// Only draw tabbar if it exists (legacy versions might not have it)
		if (tabbar.is_valid()) {
			tabbar.draw(position);
		}

		for (auto slot : Slots[tab])
			slot.draw(position);

		if (tab == Buttons::BT_TAB0)
		{
			if (!hasPendantSlot)
				disabled.draw(position + iconpositions[EquipSlot::Id::PENDANT2]);

			if (!hasPocketSlot)
				disabled.draw(position + iconpositions[EquipSlot::Id::POCKET]);

			for (auto iter : icons)
				if (iter.second)
					iter.second->draw(position + iconpositions[iter.first] + Point<int16_t>(4, 4));
		}
		else if (tab == Buttons::BT_TAB2)
		{
			disabled2.draw(position + Point<int16_t>(113, 57));
			disabled2.draw(position + Point<int16_t>(113, 106));
			disabled2.draw(position + Point<int16_t>(113, 155));
		}
	}

	Button::State UIEquipInventory::button_pressed(uint16_t id)
	{
		switch (id)
		{
		case Buttons::BT_CLOSE:
			toggle_active();
			break;
		case Buttons::BT_TAB0:
		case Buttons::BT_TAB1:
		case Buttons::BT_TAB2:
		case Buttons::BT_TAB3:
			change_tab(id);

			return Button::State::IDENTITY;
		default:
			break;
		}

		return Button::State::NORMAL;
	}

	void UIEquipInventory::update_slot(EquipSlot::Id slot)
	{
		if (int32_t item_id = inventory.get_item_id(InventoryType::Id::EQUIPPED, slot))
		{
			const Texture& texture = ItemData::get(item_id).get_icon(false);

			icons[slot] = std::make_unique<Icon>(
				std::make_unique<EquipIcon>(slot),
				texture,
				-1
				);
		}
		else if (icons[slot])
		{
			icons[slot].release();
		}

		clear_tooltip();
	}

	void UIEquipInventory::load_icons()
	{
		icons.clear();

		for (auto iter : EquipSlot::values)
			update_slot(iter);
	}

	Cursor::State UIEquipInventory::send_cursor(bool pressed, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(pressed, cursorpos);

		if (dragged)
		{
			clear_tooltip();

			return dstate;
		}

		EquipSlot::Id slot = slot_by_position(cursorpos);

		if (auto icon = icons[slot].get())
		{
			if (pressed)
			{
				icon->start_drag(cursorpos - position - iconpositions[slot]);

				UI::get().drag_icon(icon);

				clear_tooltip();

				return Cursor::State::GRABBING;
			}
			else
			{
				show_equip(slot);

				return Cursor::State::CANGRAB;
			}
		}
		else
		{
			clear_tooltip();

			return Cursor::State::IDLE;
		}
	}

	void UIEquipInventory::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				toggle_active();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				uint16_t newtab = tab + 1;

				if (newtab >= Buttons::BT_TABE)
					newtab = Buttons::BT_TAB0;

				change_tab(newtab);
			}
		}
	}

	UIElement::Type UIEquipInventory::get_type() const
	{
		return TYPE;
	}

	void UIEquipInventory::doubleclick(Point<int16_t> cursorpos)
	{
		EquipSlot::Id slot = slot_by_position(cursorpos);

		if (icons[slot])
			if (int16_t freeslot = inventory.find_free_slot(InventoryType::Id::EQUIP))
				UnequipItemPacket(slot, freeslot).dispatch();
	}

	bool UIEquipInventory::is_in_range(Point<int16_t> cursorpos) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + dimension);

		Rectangle<int16_t> totem_bounds = Rectangle<int16_t>(position, position + totem_dimensions);
		totem_bounds.shift(totem_adj);

		return bounds.contains(cursorpos) || totem_bounds.contains(cursorpos);
	}

	bool UIEquipInventory::send_icon(const Icon& icon, Point<int16_t> cursorpos)
	{
		if (EquipSlot::Id slot = slot_by_position(cursorpos))
			icon.drop_on_equips(slot);

		return true;
	}

	void UIEquipInventory::toggle_active()
	{
		clear_tooltip();

		UIElement::toggle_active();
	}

	void UIEquipInventory::modify(int16_t pos, int8_t mode, int16_t arg)
	{
		EquipSlot::Id eqpos = EquipSlot::by_id(pos);
		EquipSlot::Id eqarg = EquipSlot::by_id(arg);

		switch (mode)
		{
		case 0:
		case 3:
			update_slot(eqpos);
			break;
		case 2:
			update_slot(eqpos);
			update_slot(eqarg);
			break;
		}
	}

	void UIEquipInventory::show_equip(EquipSlot::Id slot)
	{
		UI::get().show_equip(Tooltip::Parent::EQUIPINVENTORY, slot);
	}

	void UIEquipInventory::clear_tooltip()
	{
		UI::get().clear_tooltip(Tooltip::Parent::EQUIPINVENTORY);
	}

	EquipSlot::Id UIEquipInventory::slot_by_position(Point<int16_t> cursorpos) const
	{
		if (tab != Buttons::BT_TAB0)
			return EquipSlot::Id::NONE;

		for (auto iter : iconpositions)
		{
			Rectangle<int16_t> iconrect = Rectangle<int16_t>(
				position + iter.second,
				position + iter.second + Point<int16_t>(32, 32)
				);

			if (iconrect.contains(cursorpos))
				return iter.first;
		}

		return EquipSlot::Id::NONE;
	}

	void UIEquipInventory::change_tab(uint16_t tabid)
	{
		uint8_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			clear_tooltip();

			buttons[oldtab]->set_state(Button::State::NORMAL);
			buttons[tab]->set_state(Button::State::PRESSED);

			if (tab == Buttons::BT_TAB0)
				buttons[Buttons::BT_SLOT]->set_active(true);
			else
				buttons[Buttons::BT_SLOT]->set_active(false);

			if (tab == Buttons::BT_TAB2)
			{
				buttons[Buttons::BT_CONSUMESETTING]->set_active(true);
				buttons[Buttons::BT_EXCEPTION]->set_active(true);
			}
			else
			{
				buttons[Buttons::BT_CONSUMESETTING]->set_active(false);
				buttons[Buttons::BT_EXCEPTION]->set_active(false);
			}

			if (tab == Buttons::BT_TAB3)
				buttons[Buttons::BT_SHOP]->set_active(true);
			else
				buttons[Buttons::BT_SHOP]->set_active(false);
		}
	}

	UIEquipInventory::EquipIcon::EquipIcon(int16_t s)
	{
		source = s;
	}

	void UIEquipInventory::EquipIcon::drop_on_stage() const
	{
		Sound(Sound::Name::DRAGEND).play();
	}

	void UIEquipInventory::EquipIcon::drop_on_equips(EquipSlot::Id slot) const
	{
		if (source == slot)
			Sound(Sound::Name::DRAGEND).play();
	}

	bool UIEquipInventory::EquipIcon::drop_on_items(InventoryType::Id tab, EquipSlot::Id eqslot, int16_t slot, bool equip) const
	{
		if (tab != InventoryType::Id::EQUIP)
		{
			if (auto iteminventory = UI::get().get_element<UIItemInventory>())
			{
				if (iteminventory->is_active())
				{
					iteminventory->change_tab(InventoryType::Id::EQUIP);
					return false;
				}
			}
		}

		if (equip)
		{
			if (eqslot == source)
				EquipItemPacket(slot, eqslot).dispatch();
		}
		else
		{
			UnequipItemPacket(source, slot).dispatch();
		}

		return true;
	}

	Icon::IconType UIEquipInventory::EquipIcon::get_type()
	{
		return Icon::IconType::EQUIP;
	}
}