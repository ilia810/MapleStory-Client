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
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"

#include "../Components/Textfield.h"

#include "../../Template/BoolPair.h"

namespace ms
{
	// Race selection screen - v83/v87 implementation
	class UIRaceSelect : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::RACESELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIRaceSelect();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		bool check_name(std::string name) const;
		void send_naming_result(bool nameused);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void show_charselect();
		void show_worldselect();
		std::string to_lower(std::string value) const;

		enum Buttons : uint16_t
		{
			BtStart,
			BtPreview,
			CLASS0,    // Explorer
			CLASS1,    // Cygnus Knight
			CLASS2     // Aran
		};

		// v83/v87 UI elements
		Text version;
		Point<int16_t> version_pos;
		
		// Class icon textures (normal, hover, pressed)
		Texture explorerN, explorerH, explorerP;
		Texture knightN, knightH, knightP;
		Texture aranN, aranH, aranP;
		
		// Class icon positions
		Point<int16_t> explorerPos;
		Point<int16_t> knightPos;
		Point<int16_t> aranPos;
		
		// UI state
		uint16_t selected_class;
		bool mouseover[3]; // [Explorer, Knight, Aran]
	};
}