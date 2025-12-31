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
#include "Cursor.h"

#include "../Constants.h"
#include "../Gameplay/Stage.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/GraphicsGL.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	// Static debug mode flag - toggle with F11
	bool Cursor::debug_mode = false;

	Cursor::Cursor() :
		coord_text(Text::Font::A18B, Text::Alignment::LEFT, Color::Name::WHITE, Text::Background::NAMETAG),
		map_coord_text(Text::Font::A18B, Text::Alignment::LEFT, Color::Name::YELLOW, Text::Background::NAMETAG)
	{
		state = Cursor::State::IDLE;
		hide_counter = 0;
	}

	void Cursor::init()
	{
		nl::node src = nl::nx::UI["Basic.img"]["Cursor"];

		for (auto iter : animations)
			iter.second = src[iter.first];
	}

	void Cursor::draw(float alpha) const
	{
		constexpr int64_t HIDE_AFTER = HIDE_TIME / Constants::TIMESTEP;

		if (debug_mode) {
			int16_t x = position.x();
			int16_t y = position.y();

			// Magnifier settings
			constexpr int16_t SOURCE_SIZE = 128;   // Capture 128x128 pixels around cursor
			constexpr int16_t ZOOM_FACTOR = 3;     // 3x zoom
			constexpr int16_t MAG_SIZE = SOURCE_SIZE * ZOOM_FACTOR;  // 384x384 output
			constexpr int16_t MAG_OFFSET = 25;     // Distance from cursor to magnifier
			constexpr int16_t BORDER = 2;

			// Get screen dimensions
			int16_t screen_width = Constants::Constants::get().get_viewwidth();
			int16_t screen_height = Constants::Constants::get().get_viewheight();

			// Calculate dynamic magnifier position
			// Default: bottom-right of cursor
			int16_t mag_x = x + MAG_OFFSET;
			int16_t mag_y = y + MAG_OFFSET;

			// Check if magnifier would go off right edge
			if (mag_x + MAG_SIZE + BORDER > screen_width) {
				// Place on left side of cursor
				mag_x = x - MAG_OFFSET - MAG_SIZE;
			}

			// Check if magnifier would go off bottom edge
			if (mag_y + MAG_SIZE + BORDER > screen_height) {
				// Place above cursor
				mag_y = y - MAG_OFFSET - MAG_SIZE;
			}

			// Check if magnifier would go off left edge
			if (mag_x - BORDER < 0) {
				mag_x = BORDER;
			}

			// Check if magnifier would go off top edge
			if (mag_y - BORDER < 0) {
				mag_y = BORDER;
			}

			// Draw the magnifier
			GraphicsGL::get().draw_magnifier(x, y, SOURCE_SIZE, ZOOM_FACTOR, mag_x, mag_y);

			// Draw crosshair lines
			int16_t line_length = 15;

			// Horizontal line (size = length * 2, not vertical)
			ColorLine hline(line_length * 2, Color::Name::RED, 1.0f, false);
			hline.draw(DrawArgument(Point<int16_t>(x - line_length, y)));

			// Vertical line (size = length * 2, vertical)
			ColorLine vline(line_length * 2, Color::Name::RED, 1.0f, true);
			vline.draw(DrawArgument(Point<int16_t>(x, y - line_length)));

			// Draw coordinates text (screen and map positions)
			Point<int16_t> cam_pos = Stage::get().get_camera_position();
			int16_t map_x = x - cam_pos.x();
			int16_t map_y = y - cam_pos.y();

			std::string screen_coords = "Screen: (" + std::to_string(x) + ", " + std::to_string(y) + ")";
			std::string map_coords = "Map: (" + std::to_string(map_x) + ", " + std::to_string(map_y) + ")";

			coord_text.change_text(screen_coords);
			coord_text.draw(position + Point<int16_t>(10, 10));

			map_coord_text.change_text(map_coords);
			map_coord_text.draw(position + Point<int16_t>(10, 30));
		} else if (hide_counter < HIDE_AFTER) {
			animations[state].draw(position, alpha);
		}
	}

	void Cursor::update()
	{
		animations[state].update();

		switch (state)
		{
		case Cursor::State::CANCLICK:
		case Cursor::State::CANCLICK2:
		case Cursor::State::CANGRAB:
		case Cursor::State::CLICKING:
		case Cursor::State::GRABBING:
			hide_counter = 0;
			break;
		default:
			hide_counter++;
			break;
		}
	}

	void Cursor::set_state(State s)
	{
		if (state != s)
		{
			state = s;

			animations[state].reset();
			hide_counter = 0;
		}
	}

	void Cursor::set_position(Point<int16_t> cursor_position)
	{
		position = cursor_position;
		hide_counter = 0;
	}

	Cursor::State Cursor::get_state() const
	{
		return state;
	}

	Point<int16_t> Cursor::get_position() const
	{
		return position;
	}
}