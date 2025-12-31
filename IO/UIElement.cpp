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
#include "UIElement.h"
#include "UIScale.h"

#include "../Audio/Audio.h"
#include "../Constants.h"
#include <iostream>

namespace ms
{
	UIElement::UIElement(Point<int16_t> p, Point<int16_t> d, bool a) : position(p), dimension(d), active(a) {}
	UIElement::UIElement(Point<int16_t> p, Point<int16_t> d) : UIElement(p, d, true) {}
	UIElement::UIElement() : UIElement(Point<int16_t>(), Point<int16_t>()) {}

	void UIElement::draw(float alpha) const
	{
		draw_sprites(alpha);
		draw_buttons(alpha);
	}

	void UIElement::draw_sprites(float alpha) const
	{
		float scale = UIScale::get().get_scale();

		// If scale is 1.0, scaling disabled, or element opts-out, use unscaled drawing
		if (scale == 1.0f || !should_scale())
		{
			for (const Sprite& sprite : sprites)
			{
				sprite.draw(position, alpha);
			}
		}
		else
		{
			// Apply scaling: scale both position and sprite size
			DrawArgument scale_args = UIScale::get().get_scale_args();
			Point<int16_t> scaled_pos = UIScale::get().ui_to_screen(position);

			for (const Sprite& sprite : sprites)
			{
				sprite.draw(scaled_pos, alpha, scale_args);
			}
		}
	}

	void UIElement::draw_buttons(float) const
	{
		float scale = UIScale::get().get_scale();

		// If scale is 1.0, scaling disabled, or element opts-out, use unscaled drawing
		if (scale == 1.0f || !should_scale())
		{
			for (auto& iter : buttons)
				if (const Button* button = iter.second.get())
					button->draw(position);
		}
		else
		{
			// Apply scaling
			DrawArgument scale_args = UIScale::get().get_scale_args();

			for (auto& iter : buttons)
				if (const Button* button = iter.second.get())
					button->draw(position, scale_args);
		}
	}

	void UIElement::update()
	{
		for (auto& sprite : sprites)
			sprite.update();

		for (auto& iter : buttons)
			if (Button* button = iter.second.get())
				button->update();
	}

	void UIElement::makeactive()
	{
		active = true;
	}

	void UIElement::deactivate()
	{
		active = false;
	}

	bool UIElement::is_active() const
	{
		return active;
	}

	void UIElement::toggle_active()
	{
		if (active)
			deactivate();
		else
			makeactive();
	}

	bool UIElement::is_in_range(Point<int16_t> cursor_position) const
	{
		// Transform cursor from screen space to UI space if scaling is active and element scales
		Point<int16_t> ui_cursor = should_scale() ? UIScale::get().screen_to_ui(cursor_position) : cursor_position;
		auto bounds = Rectangle<int16_t>(position, position + dimension);

		return bounds.contains(ui_cursor);
	}

	void UIElement::remove_cursor()
	{
		for (auto& btit : buttons)
		{
			auto button = btit.second.get();

			if (button && button->get_state() == Button::State::MOUSEOVER)
				button->set_state(Button::State::NORMAL);
		}
	}

	Cursor::State UIElement::send_cursor(bool clicked, Point<int16_t> cursor_position)
	{
		// Transform cursor from screen space to UI space if scaling is active and element scales
		Point<int16_t> ui_cursor = should_scale() ? UIScale::get().screen_to_ui(cursor_position) : cursor_position;

		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;

		for (auto& btit : buttons)
		{
			if (btit.second && btit.second->is_active() && btit.second->bounds(position).contains(ui_cursor))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();

					btit.second->set_state(Button::State::MOUSEOVER);
					ret = Cursor::State::CANCLICK;
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();

						btit.second->set_state(button_pressed(btit.first));

						ret = Cursor::State::IDLE;
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
			}
			else if (btit.second && btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return ret;
	}

	UIElement::ComponentInfo UIElement::get_component_at(Point<int16_t> cursor_position) const
	{
		// Transform cursor from screen space to UI space if scaling is active and element scales
		Point<int16_t> ui_cursor = should_scale() ? UIScale::get().screen_to_ui(cursor_position) : cursor_position;

		ComponentInfo info;

		// Check buttons first (they're on top)
		for (const auto& btit : buttons)
		{
			if (btit.second && btit.second->is_active())
			{
				Rectangle<int16_t> btn_bounds = btit.second->bounds(position);
				if (btn_bounds.contains(ui_cursor))
				{
					info.type = ComponentInfo::BUTTON;
					info.id = btit.first;
					info.position = Point<int16_t>(btn_bounds.left(), btn_bounds.top());
					info.dimension = Point<int16_t>(btn_bounds.width(), btn_bounds.height());
					info.name = "Button #" + std::to_string(btit.first);
					return info;
				}
			}
		}

		// Check sprites
		int sprite_idx = 0;
		for (const auto& sprite : sprites)
		{
			// Sprites don't have a simple bounds check, but we can approximate
			// using their position and the element's position
			sprite_idx++;
		}

		// If nothing specific found, it's the background
		if (is_in_range(cursor_position))
		{
			info.type = ComponentInfo::BACKGROUND;
			info.position = position;
			info.dimension = dimension;
			info.name = "Background";
		}

		return info;
	}

	void UIElement::clamp_position_to_screen()
	{
		int16_t screen_width = Constants::Constants::get().get_viewwidth();
		int16_t screen_height = Constants::Constants::get().get_viewheight();

		// Use UIScale to clamp position, taking scaling into account
		position = UIScale::get().clamp_to_screen(position, dimension, screen_width, screen_height);
	}
}