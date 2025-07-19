//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../../Graphics/Text.h"
#include <vector>
#include <functional>

namespace ms
{
	class UIMapEditorToolbar : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAP_EDITOR_TOOLBAR;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		
		UIMapEditorToolbar();
		
		void draw(float inter) const override;
		void update() override;
		
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		UIElement::Type get_type() const override { return TYPE; }
		
		enum ToolButtons : uint16_t
		{
			TB_NEW = 0,
			TB_OPEN,
			TB_SAVE,
			TB_SAVEAS,
			TB_SEPARATOR1,
			TB_UNDO,
			TB_REDO,
			TB_SEPARATOR2,
			TB_SELECT,
			TB_PAINT,
			TB_ERASE,
			TB_FILL,
			TB_SEPARATOR3,
			TB_GRID,
			TB_MAXIMIZE,
			TB_PLAYTEST,
			TB_COUNT
		};
		
		// Get currently selected tool
		ToolButtons get_selected_tool() const { return selected_tool; }
		
		// Check if grid is currently toggled
		bool is_grid_visible() const { return grid_visible; }
		
		// Set callbacks for toolbar actions
		void set_new_callback(std::function<void()> callback) { on_new = callback; }
		void set_open_callback(std::function<void()> callback) { on_open = callback; }
		void set_save_callback(std::function<void()> callback) { on_save = callback; }
		void set_save_as_callback(std::function<void()> callback) { on_save_as = callback; }
		void set_playtest_callback(std::function<void()> callback) { on_playtest = callback; }
		void set_asset_filter_callback(std::function<void(const std::string&)> callback) { on_asset_filter = callback; }
		
	protected:
		Button::State button_pressed(uint16_t buttonid) override;
		
	private:
		void toggle_grid();
		
		struct ToolButton
		{
			ToolButtons id;
			std::string label;
			std::string tooltip;
			Point<int16_t> position;
			Point<int16_t> size;
			bool enabled;
			bool pressed;
			bool separator;
			
			ToolButton(ToolButtons id, const std::string& label, const std::string& tooltip) :
				id(id), label(label), tooltip(tooltip), 
				size(40, 40), enabled(true), pressed(false), separator(false) {}
				
			ToolButton() : id(TB_COUNT), size(10, 40), enabled(false), pressed(false), separator(true) {}
		};
		
		void init_buttons();
		void draw_button(const ToolButton& button, float alpha) const;
		ToolButtons get_button_at(Point<int16_t> pos) const;
		
		std::vector<ToolButton> buttons;
		ToolButtons selected_tool;
		ToolButtons hovered_button;
		bool grid_visible;
		
		// Visual elements
		Text button_text;
		Color toolbar_bg;
		Color button_normal;
		Color button_hover;
		Color button_pressed_color;
		Color button_disabled;
		
		// Callbacks
		std::function<void()> on_new;
		std::function<void()> on_open;
		std::function<void()> on_save;
		std::function<void()> on_save_as;
		std::function<void()> on_playtest;
		std::function<void(const std::string&)> on_asset_filter;
	};
}