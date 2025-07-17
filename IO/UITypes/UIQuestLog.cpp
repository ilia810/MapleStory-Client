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
#include "UIQuestLog.h"

#include "../Components/MapleButton.h"

#include <iostream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIQuestLog::UIQuestLog(const QuestLog& ql) : UIDragElement<PosQUEST>(), questlog(ql)
	{
		tab = Buttons::TAB0;

		nl::node close = nl::nx::UI["Basic.img"]["BtClose3"];
		
		// v92: Use UIWindow.img structure directly
		nl::node quest = nl::nx::UI["UIWindow.img"]["Quest"];
		nl::node list = quest["list"];
		
		if (!quest || !list) {
			// If no Quest window assets found, create minimal window with defaults
			dimension = Point<int16_t>(300, 400);
			dragarea = Point<int16_t>(300, 20);
			// Create minimal close button
			if (close) buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(275, 6));
			return;
		}

		nl::node backgrnd = list["backgrnd"];

		// Add sprites only if nodes exist
		if (backgrnd) sprites.emplace_back(backgrnd);
		if (list["backgrnd2"]) sprites.emplace_back(list["backgrnd2"]);

		// Add notice sprites only if nodes exist
		if (list["notice0"]) notice_sprites.emplace_back(list["notice0"]);
		if (list["notice1"]) notice_sprites.emplace_back(list["notice1"]);
		if (list["notice2"]) notice_sprites.emplace_back(list["notice2"]);

		nl::node taben = list["Tab"]["enabled"];
		nl::node tabdis = list["Tab"]["disabled"];

		// Create buttons only if nodes exist
		if (tabdis["0"] && taben["0"]) buttons[Buttons::TAB0] = std::make_unique<TwoSpriteButton>(tabdis["0"], taben["0"]);
		if (tabdis["1"] && taben["1"]) buttons[Buttons::TAB1] = std::make_unique<TwoSpriteButton>(tabdis["1"], taben["1"]);
		if (tabdis["2"] && taben["2"]) buttons[Buttons::TAB2] = std::make_unique<TwoSpriteButton>(tabdis["2"], taben["2"]);
		if (close) buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(close, Point<int16_t>(275, 6));
		if (list["BtSearch"]) buttons[Buttons::SEARCH] = std::make_unique<MapleButton>(list["BtSearch"]);
		if (list["BtAllLevel"]) buttons[Buttons::ALL_LEVEL] = std::make_unique<MapleButton>(list["BtAllLevel"]);
		if (list["BtMyLocation"]) buttons[Buttons::MY_LOCATION] = std::make_unique<MapleButton>(list["BtMyLocation"]);

		search_area = list["searchArea"];

		int16_t search_limit = 19;

		search = Textfield(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, Rectangle<int16_t>(get_search_pos(), get_search_pos() + get_search_dim()), search_limit);
		placeholder = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BOULDER, "Enter the quest name.");

		slider = Slider(Slider::Type::DEFAULT_SILVER, Range<int16_t>(0, 279), 150, 20, 5, [](bool) {});

		change_tab(tab);

		if (backgrnd) {
			dimension = Texture(backgrnd).get_dimensions();
			dragarea = Point<int16_t>(dimension.x(), 20);
		} else {
			dimension = Point<int16_t>(400, 300); // Default size
			dragarea = Point<int16_t>(400, 20);
		}
	}

	void UIQuestLog::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		Point<int16_t> notice_position = Point<int16_t>(0, 26);

		// Only draw notice sprites if they exist and tab is valid
		if (tab < notice_sprites.size() && notice_sprites.size() > 0) {
			if (tab == Buttons::TAB0)
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(9, 0), alpha);
			else if (tab == Buttons::TAB1)
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(0, 0), alpha);
			else
				notice_sprites[tab].draw(position + notice_position + Point<int16_t>(-10, 0), alpha);
		}

		if (tab != Buttons::TAB2)
		{
			search_area.draw(position);
			search.draw(Point<int16_t>(4, -4), Point<int16_t>(2, -2));

			if (search.get_state() == Textfield::State::NORMAL && search.empty())
				placeholder.draw(position + Point<int16_t>(39, 51));
		}

		slider.draw(position + Point<int16_t>(126, 75));

		UIElement::draw_buttons(alpha);
	}

	void UIQuestLog::update()
	{
		search.update(get_search_pos(), get_search_dim());
	}

	void UIQuestLog::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				deactivate();
			}
			else if (keycode == KeyAction::Id::TAB)
			{
				uint16_t new_tab = tab;

				if (new_tab < Buttons::TAB2)
					new_tab++;
				else
					new_tab = Buttons::TAB0;

				change_tab(new_tab);
			}
		}
	}

	Cursor::State UIQuestLog::send_cursor(bool clicking, Point<int16_t> cursorpos)
	{
		if (Cursor::State new_state = search.send_cursor(cursorpos, clicking))
			return new_state;

		return UIDragElement::send_cursor(clicking, cursorpos);
	}

	UIElement::Type UIQuestLog::get_type() const
	{
		return TYPE;
	}

	Button::State UIQuestLog::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case Buttons::TAB0:
			case Buttons::TAB1:
			case Buttons::TAB2:
				change_tab(buttonid);

				return Button::State::IDENTITY;
			case Buttons::CLOSE:
				deactivate();
				return Button::State::NORMAL;
			default:
				return Button::State::DISABLED;
		}
	}

	void UIQuestLog::change_tab(uint16_t tabid)
	{
		uint16_t oldtab = tab;
		tab = tabid;

		if (oldtab != tab)
		{
			// Check if old tab button exists before setting state
			if (buttons[Buttons::TAB0 + oldtab]) {
				buttons[Buttons::TAB0 + oldtab]->set_state(Button::State::NORMAL);
			}
			
			// Check if buttons exist before setting active state
			if (buttons[Buttons::MY_LOCATION]) {
				buttons[Buttons::MY_LOCATION]->set_active(tab == Buttons::TAB0);
			}
			if (buttons[Buttons::ALL_LEVEL]) {
				buttons[Buttons::ALL_LEVEL]->set_active(tab == Buttons::TAB0);
			}
			if (buttons[Buttons::SEARCH]) {
				buttons[Buttons::SEARCH]->set_active(tab != Buttons::TAB2);
			}

			if (tab == Buttons::TAB2)
				search.set_state(Textfield::State::DISABLED);
			else
				search.set_state(Textfield::State::NORMAL);
		}

		// Check if new tab button exists before setting state
		if (buttons[Buttons::TAB0 + tab]) {
			buttons[Buttons::TAB0 + tab]->set_state(Button::State::PRESSED);
		}
	}

	Point<int16_t> UIQuestLog::get_search_pos()
	{
		Point<int16_t> search_area_origin = search_area.get_origin().abs();
		Point<int16_t> search_pos_adj = Point<int16_t>(25, 4);

		return position + search_area_origin + search_pos_adj;
	}

	Point<int16_t> UIQuestLog::get_search_dim()
	{
		Point<int16_t> adjust = Point<int16_t>(-75, -8);

		return search_area.get_dimensions() + adjust;
	}
}