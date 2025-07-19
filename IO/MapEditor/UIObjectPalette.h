//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../../Graphics/Text.h"
#include "../../Template/Point.h"
#include <vector>
#include <string>

namespace ms
{
	// UI element for selecting objects to place (NPCs, mobs, portals)
	class UIObjectPalette : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAP_EDITOR_OBJECT_PALETTE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		
		enum ObjectType
		{
			OBJ_PORTAL,
			OBJ_NPC,
			OBJ_MOB,
			OBJ_SPAWN
		};
		
		struct ObjectEntry
		{
			ObjectType type;
			int32_t id;
			std::string name;
			std::string description;
		};
		
		UIObjectPalette();
		
		void draw(float inter) const override;
		void update() override;
		
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_scroll(double yoffset) override;
		
		UIElement::Type get_type() const override { return TYPE; }
		
		// Get currently selected object
		const ObjectEntry* get_selected_object() const;
		ObjectType get_selected_type() const { return selected_type; }
		
		// Set object type filter
		void set_object_type(ObjectType type);
		
	protected:
		Button::State button_pressed(uint16_t buttonid) override;
		
	private:
		void init_objects();
		void draw_entry(const ObjectEntry& entry, Point<int16_t> position, bool selected) const;
		int16_t get_entry_at(Point<int16_t> pos) const;
		
		// Object data
		std::vector<ObjectEntry> objects;
		std::vector<ObjectEntry> filtered_objects;
		
		// Selection state
		ObjectType selected_type;
		int16_t selected_index;
		int16_t hovered_index;
		
		// Scrolling
		int16_t scroll_offset;
		int16_t max_scroll;
		
		// Visual settings
		static constexpr int16_t ENTRY_HEIGHT = 25;
		static constexpr int16_t PALETTE_WIDTH = 250;
		static constexpr int16_t VISIBLE_ENTRIES = 12;
		
		// Visual elements
		Text entry_text;
		Color palette_bg;
		Color entry_normal;
		Color entry_hover;
		Color entry_selected;
		
		// Tab buttons for object types
		enum TabButton
		{
			TAB_PORTAL,
			TAB_NPC,
			TAB_MOB,
			TAB_SPAWN,
			TAB_COUNT
		};
	};
}