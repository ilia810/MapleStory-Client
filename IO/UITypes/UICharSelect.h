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
#pragma once

#include "../UIElement.h"
#include "../Components/AreaButton.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "../../Character/Look/CharLook.h"
#include "../../Net/Login.h"

namespace ms
{
	class UICharSelect : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHARSELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UICharSelect(std::vector<CharEntry> characters, int8_t characters_count, int32_t slots, int8_t require_pic);

		void draw(float inter) const override;
		void update() override;

		void doubleclick(Point<int16_t> cursorpos) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void add_character(CharEntry&& character);
		void post_add_character();
		void remove_character(int32_t id);

		const CharEntry& get_character(int32_t id);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void update_buttons();
		void select_character(int32_t index);

		enum Buttons : uint16_t
		{
			BtEnter,
			CHARACTER_SLOT0,
			NUM_BUTTONS
		};

		// Frame (stretched to 808x608)
		Texture frame;
		Point<int16_t> frame_stretch;

		// Character slot texture from ViewAllChar/Select
		Texture char_slot;
		Point<int16_t> char_slot_pos;
		Rectangle<int16_t> char_slot_bounds;

		// Character data
		std::vector<CharEntry> characters;
		std::vector<CharLook> charlooks;
		std::vector<Text> nametags;

		int8_t characters_count;
		int32_t slots;
		int8_t require_pic;
		int32_t selected_character;
	};
}
