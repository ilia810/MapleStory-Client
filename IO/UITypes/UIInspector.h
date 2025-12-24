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
#include "../../Graphics/Text.h"
#include "../../Template/Rectangle.h"

#include <string>
#include <vector>

namespace ms
{
	// UI Inspector - Debug tool for inspecting UI element properties
	// Toggle with F12 key. Click on any UI element to see its metadata.
	class UIInspector : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::NUM_TYPES;  // Special type
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIInspector();

		void draw(float alpha) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursor_position) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		// Set the element to inspect
		void inspect_element(UIElement* element, UIElement::Type type);

		// Clear the inspected element
		void clear_inspection();

		// Check if inspection mode is active
		bool is_inspecting() const;

		// Get element type name as string
		static std::string get_type_name(UIElement::Type type);

	private:
		void update_info_text();
		void draw_element_bounds() const;
		void draw_info_panel() const;

		// The element currently being inspected
		UIElement* inspected_element;
		UIElement::Type inspected_type;

		// Display position for info panel
		Point<int16_t> info_position;

		// Info text lines
		std::vector<Text> info_lines;

		// Cursor position for highlighting
		Point<int16_t> cursor_pos;

		// Whether we're actively inspecting
		bool inspecting;

		// Colors for drawing
		static constexpr float BOUNDS_COLOR_R = 1.0f;
		static constexpr float BOUNDS_COLOR_G = 0.0f;
		static constexpr float BOUNDS_COLOR_B = 0.0f;
		static constexpr float BOUNDS_COLOR_A = 0.8f;

		static constexpr float PANEL_BG_R = 0.0f;
		static constexpr float PANEL_BG_G = 0.0f;
		static constexpr float PANEL_BG_B = 0.0f;
		static constexpr float PANEL_BG_A = 0.85f;
	};

	// Global inspection mode manager
	class InspectorMode
	{
	public:
		static bool is_enabled();
		static void toggle();
		static void enable();
		static void disable();

		static UIElement* get_element_at(Point<int16_t> cursor_position);
		static void set_hovered_element(UIElement* element, UIElement::Type type);
		static UIElement* get_hovered_element();
		static UIElement::Type get_hovered_type();

	private:
		static bool enabled;
		static UIElement* hovered_element;
		static UIElement::Type hovered_type;
	};
}
