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
namespace EquipGrid {
	const int16_t StartX = 5;
	const int16_t StartY = 34;
	const int16_t SlotSize = 33;
}

namespace ms
{
	UIEquipInventory::UIEquipInventory(const Inventory& invent) : UIDragElement<PosEQINV>(), inventory(invent), tab(Buttons::BT_TAB1), hasPendantSlot(false), hasPocketSlot(false), pet_active(true)
	{
		// Grid Configuration
		

		// Coordinate calculation helper
		auto getPos = [] (int col, int row) {
			return Point<int16_t>(
				EquipGrid::StartX + (col * EquipGrid::SlotSize),
				EquipGrid::StartY + (row * EquipGrid::SlotSize)
			);
			};

		// Row 0
		iconpositions[EquipSlot::Id::HAT] = getPos(1, 0); // CAP

		// Row 1
		iconpositions[EquipSlot::Id::MEDAL] = getPos(0, 1); // MEDAL
		iconpositions[EquipSlot::Id::FACE] = getPos(1, 1); // FOREHEAD
		iconpositions[EquipSlot::Id::RING1] = getPos(3, 1); // RING
		iconpositions[EquipSlot::Id::RING2] = getPos(4, 1); // RING

		// Row 2
		iconpositions[EquipSlot::Id::EYEACC] = getPos(2, 2); // EYE ACC
		iconpositions[EquipSlot::Id::EARACC] = getPos(3, 2); // EAR ACC
		iconpositions[EquipSlot::Id::SHOULDER] = getPos(4, 2); // SHOULDER

		// Row 3
		iconpositions[EquipSlot::Id::CAPE] = getPos(0, 3); // MANTLE
		iconpositions[EquipSlot::Id::TOP] = getPos(1, 3); // CLOTHES
		iconpositions[EquipSlot::Id::PENDANT1] = getPos(2, 3); // PENDANT
		iconpositions[EquipSlot::Id::WEAPON] = getPos(3, 3); // WEAPON
		iconpositions[EquipSlot::Id::SUBWEAPON] = getPos(4, 3); // WEAPON (Sub)

		// Row 4
		iconpositions[EquipSlot::Id::GLOVES] = getPos(0, 4); // GLOVES
		iconpositions[EquipSlot::Id::BOTTOM] = getPos(1, 4); // PANTS
		iconpositions[EquipSlot::Id::BELT] = getPos(2, 4); // BELT
		iconpositions[EquipSlot::Id::RING3] = getPos(3, 4); // RING
		iconpositions[EquipSlot::Id::RING4] = getPos(4, 4); // RING

		// Row 5
		iconpositions[EquipSlot::Id::SHOES] = getPos(2, 5); // SHOES

		// Row 6
		iconpositions[EquipSlot::Id::TAMEDMOB] = getPos(0, 6); // TAMING MOB
		//iconpositions[EquipSlot::Id::PETMP] = getPos(4, 5); // PET MP
		//iconpositions[EquipSlot::Id::MOBEQUIP] = getPos(2, 6); // MOB EQUIP
		//iconpositions[EquipSlot::Id::PETHP] = getPos(4, 6); // PET HP

		// Row 7
		iconpositions[EquipSlot::Id::SADDLE] = getPos(1, 7); // SADDLE

		tab_source[Buttons::BT_TAB0] = "Equip";
		tab_source[Buttons::BT_TAB1] = "Cash";
		tab_source[Buttons::BT_TAB2] = "Pet";
		tab_source[Buttons::BT_TAB3] = "Android";

		// Equip inventory is at UIWindow.img/Equip
		nl::node Equip = nl::nx::UI["UIWindow.img"]["Equip"];

		// Load background from Equip node (backgrnd is 175x291)
		nl::node main_bg = Equip["backgrnd"];

		// Get dimensions from background texture
		Point<int16_t> bg_dimensions = Point<int16_t>(175, 291);
		if (main_bg) {
			Texture bg_tex(main_bg);
			if (bg_tex.is_valid()) {
				bg_dimensions = bg_tex.get_dimensions();
			}
			// Add background to sprites - this is how other UI windows work
			sprites.emplace_back(main_bg, Point<int16_t>(0, 0));
		}

		totem_dimensions = bg_dimensions;
		totem_adj = Point<int16_t>(0, 0);

		nl::node close_node = Equip["BtClose"];

		// Close button using BtDetail texture at top right
		buttons[Buttons::BT_CLOSE] = std::make_unique<MapleButton>(close_node, Point<int16_t>(bg_dimensions.x() - 18, 5));

		// Load pet window texture - try with ["0"] child
		pet_window = Texture(Equip["pet"]["0"]);

		// BtDetail button at bottom right of equip window (opens pet equipment)
		buttons[Buttons::BT_DETAIL] = std::make_unique<MapleButton>(Equip["BtDetail"], Point<int16_t>(bg_dimensions.x() - 60, bg_dimensions.y() - 24));

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(bg_dimensions.x(), 20);

		load_icons();
		// V92: No tabs, just show equip tab
		tab = Buttons::BT_TAB0;
	}

	void UIEquipInventory::draw(float alpha) const
	{
		// Draw sprites (background) and buttons
		UIElement::draw(alpha);

		// Draw equipped items on the equip tab
		for (auto iter : icons)
			if (iter.second)
				iter.second->draw(position + iconpositions[iter.first] + Point<int16_t>(4, 4));

		// Draw pet window if active
		// Position: right of equip window, bottoms aligned
		if (pet_active) {
			// Test: draw at fixed position to the right
			Point<int16_t> pet_pos = Point<int16_t>(
				position.x() + dimension.x(),
				position.y()
			);
			pet_window.draw(DrawArgument(pet_pos));
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
		case Buttons::BT_DETAIL:
			pet_active = !pet_active;
			return Button::State::NORMAL;
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

		bool in_main = bounds.contains(cursorpos) || totem_bounds.contains(cursorpos);

		// Check pet window bounds if active
		if (pet_active && pet_window.is_valid()) {
			Point<int16_t> pet_dims = pet_window.get_dimensions();
			Point<int16_t> pet_pos = Point<int16_t>(
				position.x() + dimension.x(),
				position.y() + dimension.y() - pet_dims.y()
			);
			Rectangle<int16_t> pet_bounds = Rectangle<int16_t>(pet_pos, pet_pos + pet_dims);
			if (pet_bounds.contains(cursorpos))
				return true;
		}

		return in_main;
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
		pet_active = false;

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

			// V92: Tab buttons may not exist
			if (buttons.count(oldtab) && buttons[oldtab])
				buttons[oldtab]->set_state(Button::State::NORMAL);
			if (buttons.count(tab) && buttons[tab])
				buttons[tab]->set_state(Button::State::PRESSED);

			if (buttons.count(Buttons::BT_SLOT) && buttons[Buttons::BT_SLOT]) {
				if (tab == Buttons::BT_TAB0)
					buttons[Buttons::BT_SLOT]->set_active(true);
				else
					buttons[Buttons::BT_SLOT]->set_active(false);
			}

			if (tab == Buttons::BT_TAB2)
			{
				if (buttons.count(Buttons::BT_CONSUMESETTING) && buttons[Buttons::BT_CONSUMESETTING])
					buttons[Buttons::BT_CONSUMESETTING]->set_active(true);
				if (buttons.count(Buttons::BT_EXCEPTION) && buttons[Buttons::BT_EXCEPTION])
					buttons[Buttons::BT_EXCEPTION]->set_active(true);
			}
			else
			{
				if (buttons.count(Buttons::BT_CONSUMESETTING) && buttons[Buttons::BT_CONSUMESETTING])
					buttons[Buttons::BT_CONSUMESETTING]->set_active(false);
				if (buttons.count(Buttons::BT_EXCEPTION) && buttons[Buttons::BT_EXCEPTION])
					buttons[Buttons::BT_EXCEPTION]->set_active(false);
			}

			if (buttons.count(Buttons::BT_SHOP) && buttons[Buttons::BT_SHOP]) {
				if (tab == Buttons::BT_TAB3)
					buttons[Buttons::BT_SHOP]->set_active(true);
				else
					buttons[Buttons::BT_SHOP]->set_active(false);
			}
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