//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "UIMapEditorToolbar.h"
#include "../../Graphics/GraphicsGL.h"
#include "../UI.h"
#include "../Window.h"
#include "../../Template/Rectangle.h"
#include "../../MapleStory.h"
#include <iostream>

namespace ms
{
	UIMapEditorToolbar::UIMapEditorToolbar() :
		selected_tool(TB_SELECT),
		hovered_button(TB_COUNT),
		grid_visible(true),
		button_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK),
		toolbar_bg(static_cast<uint8_t>(230), static_cast<uint8_t>(230), static_cast<uint8_t>(230), static_cast<uint8_t>(255)),
		button_normal(static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(255)),
		button_hover(static_cast<uint8_t>(220), static_cast<uint8_t>(220), static_cast<uint8_t>(220), static_cast<uint8_t>(255)),
		button_pressed_color(static_cast<uint8_t>(160), static_cast<uint8_t>(160), static_cast<uint8_t>(160), static_cast<uint8_t>(255)),
		button_disabled(static_cast<uint8_t>(150), static_cast<uint8_t>(150), static_cast<uint8_t>(150), static_cast<uint8_t>(255))
	{
		// Position toolbar at top of screen
		position = Point<int16_t>(0, 0);
		dimension = Point<int16_t>(800, 50);
		active = true;
		
		init_buttons();
	}
	
	void UIMapEditorToolbar::init_buttons()
	{
		buttons.clear();
		
		// File operations
		buttons.emplace_back(TB_NEW, "New", "Create new map");
		buttons.emplace_back(TB_OPEN, "Open", "Open map file");
		buttons.emplace_back(TB_SAVE, "Save", "Save current map");
		buttons.emplace_back(TB_SAVEAS, "Save As", "Save map with new name");
		buttons.emplace_back(); // Separator
		
		// Edit operations
		buttons.emplace_back(TB_UNDO, "Undo", "Undo last action");
		buttons.emplace_back(TB_REDO, "Redo", "Redo last action");
		buttons.emplace_back(); // Separator
		
		// Tools
		buttons.emplace_back(TB_SELECT, "Select", "Selection tool");
		buttons.emplace_back(TB_PAINT, "Paint", "Paint tiles");
		buttons.emplace_back(TB_ERASE, "Erase", "Erase tiles");
		buttons.emplace_back(TB_FILL, "Fill", "Fill area");
		buttons.emplace_back(); // Separator
		
		// View options
		buttons.emplace_back(TB_GRID, "Grid", "Toggle grid");
		buttons.emplace_back(TB_MAXIMIZE, "Max", "Maximize window");
		buttons.emplace_back(TB_PLAYTEST, "Play", "Test map");
		
		// Calculate positions
		int16_t x = 10;
		for (auto& button : buttons)
		{
			button.position = Point<int16_t>(x, 5);
			x += button.size.x() + 5;
			
			// Select is default tool
			if (button.id == TB_SELECT)
				button.pressed = true;
				
			// Disable undo/redo initially
			if (button.id == TB_UNDO || button.id == TB_REDO)
				button.enabled = false;
				
			// Grid is initially toggled on
			if (button.id == TB_GRID)
				button.pressed = true;
		}
	}
	
	void UIMapEditorToolbar::draw(float inter) const
	{
		// Draw toolbar background
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			dimension.y(),
			toolbar_bg.r(),
			toolbar_bg.g(),
			toolbar_bg.b(),
			toolbar_bg.a()
		);
		
		// Draw bottom border
		Color border_color(static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(255));
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y() + dimension.y() - 2,
			dimension.x(),
			2,
			border_color.r(),
			border_color.g(),
			border_color.b(),
			border_color.a()
		);
		
		// Draw buttons
		for (const auto& button : buttons)
		{
			draw_button(button, 1.0f);
		}
	}
	
	void UIMapEditorToolbar::draw_button(const ToolButton& button, float alpha) const
	{
		if (button.separator)
		{
			// Draw separator line
			Color sep_color(static_cast<uint8_t>(180), static_cast<uint8_t>(180), static_cast<uint8_t>(180), static_cast<uint8_t>(255));
			GraphicsGL::get().drawrectangle(
				position.x() + button.position.x() + button.size.x() / 2 - 1,
				position.y() + button.position.y() + 5,
				2,
				button.size.y() - 10,
				sep_color.r(),
				sep_color.g(),
				sep_color.b(),
				sep_color.a()
			);
			return;
		}
		
		// Determine button color based on state
		Color btn_color = button_normal;
		if (!button.enabled)
			btn_color = button_disabled;
		else if (button.pressed || (button.id >= TB_SELECT && button.id <= TB_FILL && button.id == selected_tool))
			btn_color = button_pressed_color;
		else if (button.id == hovered_button)
			btn_color = button_hover;
		
		// Draw button background
		Point<int16_t> btn_pos = position + button.position;
		GraphicsGL::get().drawrectangle(
			btn_pos.x(),
			btn_pos.y(),
			button.size.x(),
			button.size.y(),
			btn_color.r(),
			btn_color.g(),
			btn_color.b(),
			btn_color.a() * alpha
		);
		
		// Draw button border
		Color border_color(static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(255));
		// Top
		GraphicsGL::get().drawrectangle(btn_pos.x(), btn_pos.y(), button.size.x(), 1, border_color.r(), border_color.g(), border_color.b(), border_color.a() * alpha);
		// Bottom
		GraphicsGL::get().drawrectangle(btn_pos.x(), btn_pos.y() + button.size.y() - 1, button.size.x(), 1, border_color.r(), border_color.g(), border_color.b(), border_color.a() * alpha);
		// Left
		GraphicsGL::get().drawrectangle(btn_pos.x(), btn_pos.y(), 1, button.size.y(), border_color.r(), border_color.g(), border_color.b(), border_color.a() * alpha);
		// Right
		GraphicsGL::get().drawrectangle(btn_pos.x() + button.size.x() - 1, btn_pos.y(), 1, button.size.y(), border_color.r(), border_color.g(), border_color.b(), border_color.a() * alpha);
		
		// Draw button icon/text (simplified - just first letter for now)
		if (!button.label.empty())
		{
			Text label_text(Text::Font::A12B, Text::Alignment::CENTER, 
				button.enabled ? Color::Name::BLACK : Color::Name::GRAY);
			label_text.change_text(button.label.substr(0, 1));
			label_text.draw(DrawArgument(btn_pos + Point<int16_t>(button.size.x() / 2, button.size.y() / 2)));
		}
	}
	
	void UIMapEditorToolbar::update()
	{
		// Update any animations or states
	}
	
	Cursor::State UIMapEditorToolbar::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Check if cursor is over toolbar
		Rectangle<int16_t> toolbar_bounds(position.x(), position.x() + dimension.x(), position.y(), position.y() + dimension.y());
		if (!toolbar_bounds.contains(cursorpos))
		{
			hovered_button = TB_COUNT;
			return UIElement::send_cursor(clicked, cursorpos);
		}
		
		// Find which button is hovered
		ToolButtons prev_hover = hovered_button;
		hovered_button = get_button_at(cursorpos);
		
		if (clicked && hovered_button != TB_COUNT)
		{
			button_pressed(static_cast<uint16_t>(hovered_button));
			return Cursor::State::CLICKING;
		}
		
		return (hovered_button != TB_COUNT) ? Cursor::State::CANCLICK : Cursor::State::IDLE;
	}
	
	Button::State UIMapEditorToolbar::button_pressed(uint16_t buttonid)
	{
		ToolButtons btn = static_cast<ToolButtons>(buttonid);
		
		// Handle tool selection
		if (btn >= TB_SELECT && btn <= TB_FILL)
		{
			// Deselect previous tool
			for (auto& button : buttons)
			{
				if (button.id >= TB_SELECT && button.id <= TB_FILL)
					button.pressed = false;
			}
			
			// Select new tool
			selected_tool = btn;
			for (auto& button : buttons)
			{
				if (button.id == btn)
				{
					button.pressed = true;
					break;
				}
			}
		}
		
		// Handle other buttons
		switch (btn)
		{
		case TB_NEW:
			if (on_new)
				on_new();
			break;
		case TB_OPEN:
			if (on_open)
				on_open();
			break;
		case TB_SAVE:
			if (on_save)
				on_save();
			break;
		case TB_SAVEAS:
			if (on_save_as)
				on_save_as();
			break;
		case TB_UNDO:
			// TODO: Implement undo
			// TODO: Implement undo
			break;
		case TB_REDO:
			// TODO: Implement redo
			// TODO: Implement redo
			break;
		case TB_GRID:
			// Toggle grid visibility
			toggle_grid();
			break;
		case TB_MAXIMIZE:
			// Maximize window
			Window::get().maximize_window();
			break;
		case TB_PLAYTEST:
			if (on_playtest)
				on_playtest();
			break;
		}
		
		return Button::State::NORMAL;
	}
	
	UIMapEditorToolbar::ToolButtons UIMapEditorToolbar::get_button_at(Point<int16_t> pos) const
	{
		Point<int16_t> relative_pos = pos - position;
		
		for (const auto& button : buttons)
		{
			if (button.separator)
				continue;
				
			Rectangle<int16_t> btn_bounds(
				button.position.x(),
				button.position.x() + button.size.x(),
				button.position.y(),
				button.position.y() + button.size.y()
			);
			
			if (btn_bounds.contains(relative_pos) && button.enabled)
			{
				return button.id;
			}
		}
		
		return TB_COUNT;
	}
	
	void UIMapEditorToolbar::toggle_grid()
	{
		grid_visible = !grid_visible;
		
		// Update button state
		for (auto& button : buttons)
		{
			if (button.id == TB_GRID)
			{
				button.pressed = grid_visible;
				break;
			}
		}
		
		// Notify the map editor state about grid toggle
		// This will be connected to the MapRenderer in UIStateMapEditor
		LOG(LOG_INFO, "[MapEditor] Grid visibility toggled: " << (grid_visible ? "ON" : "OFF"));
	}
}