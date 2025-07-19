//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "UIObjectPalette.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../MapleStory.h"
#include <iostream>

namespace ms
{
	UIObjectPalette::UIObjectPalette() :
		selected_type(OBJ_PORTAL),
		selected_index(-1),
		hovered_index(-1),
		scroll_offset(0),
		max_scroll(0),
		entry_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK),
		palette_bg(static_cast<uint8_t>(245), static_cast<uint8_t>(245), static_cast<uint8_t>(245), static_cast<uint8_t>(255)),
		entry_normal(static_cast<uint8_t>(255), static_cast<uint8_t>(255), static_cast<uint8_t>(255), static_cast<uint8_t>(255)),
		entry_hover(static_cast<uint8_t>(230), static_cast<uint8_t>(230), static_cast<uint8_t>(255), static_cast<uint8_t>(255)),
		entry_selected(static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(255), static_cast<uint8_t>(255))
	{
		// Position at right side of screen
		position = Point<int16_t>(550, 60);
		dimension = Point<int16_t>(PALETTE_WIDTH, ENTRY_HEIGHT * VISIBLE_ENTRIES + 40); // +40 for tabs
		active = true;
		
		init_objects();
		set_object_type(OBJ_PORTAL);
	}
	
	void UIObjectPalette::init_objects()
	{
		// Initialize with some common objects
		// In a real implementation, these would be loaded from WZ files
		
		// Portals
		objects.push_back({OBJ_PORTAL, 0, "Regular Portal", "Standard map transition"});
		objects.push_back({OBJ_PORTAL, 1, "Hidden Portal", "Invisible portal"});
		objects.push_back({OBJ_PORTAL, 2, "Town Portal", "Return to town"});
		
		// NPCs
		objects.push_back({OBJ_NPC, 1012000, "Regular Store", "General merchant"});
		objects.push_back({OBJ_NPC, 1012001, "Potion Store", "Potion merchant"});
		objects.push_back({OBJ_NPC, 1012100, "Quest NPC", "Gives quests"});
		
		// Mobs
		objects.push_back({OBJ_MOB, 100100, "Snail", "Weak mob"});
		objects.push_back({OBJ_MOB, 100101, "Blue Snail", "Slightly stronger"});
		objects.push_back({OBJ_MOB, 130100, "Jr. Balrog", "Boss monster"});
		
		// Spawn points
		objects.push_back({OBJ_SPAWN, 0, "Player Spawn", "Where players start"});
		objects.push_back({OBJ_SPAWN, 1, "Mob Spawn Area", "Where mobs respawn"});
	}
	
	void UIObjectPalette::draw(float inter) const
	{
		// Draw background
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			dimension.y(),
			palette_bg.r(),
			palette_bg.g(),
			palette_bg.b(),
			palette_bg.a()
		);
		
		// Draw tabs
		const char* tab_names[] = { "Portals", "NPCs", "Mobs", "Spawns" };
		int16_t tab_width = dimension.x() / TAB_COUNT;
		
		for (int i = 0; i < TAB_COUNT; i++)
		{
			bool is_selected = (i == selected_type);
			Color tab_color = is_selected ? entry_selected : entry_normal;
			
			GraphicsGL::get().drawrectangle(
				position.x() + i * tab_width,
				position.y(),
				tab_width,
				30,
				tab_color.r(),
				tab_color.g(),
				tab_color.b(),
				tab_color.a()
			);
			
			// Draw tab text
			Text tab_text(Text::Font::A12B, Text::Alignment::CENTER, 
				is_selected ? Color::Name::WHITE : Color::Name::BLACK);
			tab_text.change_text(tab_names[i]);
			tab_text.draw(DrawArgument(
				position + Point<int16_t>(i * tab_width + tab_width / 2, 15)
			));
		}
		
		// Draw object entries
		int16_t y_offset = 40; // Start below tabs
		int16_t visible_count = 0;
		
		for (size_t i = scroll_offset; i < filtered_objects.size() && visible_count < VISIBLE_ENTRIES; i++)
		{
			Point<int16_t> entry_pos(position.x(), position.y() + y_offset);
			bool is_selected = (i == selected_index);
			bool is_hovered = (i == hovered_index);
			
			draw_entry(filtered_objects[i], entry_pos, is_selected || is_hovered);
			
			y_offset += ENTRY_HEIGHT;
			visible_count++;
		}
		
		// Draw scroll bar if needed
		if (max_scroll > 0)
		{
			int16_t scrollbar_x = position.x() + dimension.x() - 10;
			int16_t scrollbar_y = position.y() + 40;
			int16_t scrollbar_height = ENTRY_HEIGHT * VISIBLE_ENTRIES;
			
			// Background
			GraphicsGL::get().drawrectangle(
				scrollbar_x,
				scrollbar_y,
				8,
				scrollbar_height,
				200, 200, 200, 255
			);
			
			// Thumb
			float scroll_ratio = static_cast<float>(scroll_offset) / max_scroll;
			int16_t thumb_height = scrollbar_height / 4;
			int16_t thumb_y = scrollbar_y + static_cast<int16_t>(scroll_ratio * (scrollbar_height - thumb_height));
			
			GraphicsGL::get().drawrectangle(
				scrollbar_x,
				thumb_y,
				8,
				thumb_height,
				100, 100, 100, 255
			);
		}
	}
	
	void UIObjectPalette::draw_entry(const ObjectEntry& entry, Point<int16_t> pos, bool highlighted) const
	{
		// Draw background
		Color bg_color = highlighted ? 
			(selected_index >= 0 && filtered_objects[selected_index].id == entry.id ? entry_selected : entry_hover) : 
			entry_normal;
			
		GraphicsGL::get().drawrectangle(
			pos.x(),
			pos.y(),
			dimension.x() - 10, // Leave room for scrollbar
			ENTRY_HEIGHT,
			bg_color.r(),
			bg_color.g(),
			bg_color.b(),
			bg_color.a()
		);
		
		// Draw border
		GraphicsGL::get().drawrectangle(
			pos.x(),
			pos.y() + ENTRY_HEIGHT - 1,
			dimension.x() - 10,
			1,
			200, 200, 200, 255
		);
		
		// Draw text
		Text name_text(Text::Font::A11B, Text::Alignment::LEFT, Color::Name::BLACK);
		name_text.change_text(entry.name);
		name_text.draw(DrawArgument(pos + Point<int16_t>(5, 5)));
		
		// Draw ID
		Text id_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::GRAY);
		id_text.change_text("ID: " + std::to_string(entry.id));
		id_text.draw(DrawArgument(pos + Point<int16_t>(150, 5)));
	}
	
	void UIObjectPalette::update()
	{
		// Update animations or states if needed
	}
	
	Cursor::State UIObjectPalette::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Rectangle<int16_t> bounds(position.x(), position.x() + dimension.x(), 
			position.y(), position.y() + dimension.y());
			
		if (!bounds.contains(cursorpos))
		{
			hovered_index = -1;
			return UIElement::send_cursor(clicked, cursorpos);
		}
		
		Point<int16_t> relative_pos = cursorpos - position;
		
		// Check tabs
		if (relative_pos.y() < 30)
		{
			int16_t tab_width = dimension.x() / TAB_COUNT;
			int16_t tab = relative_pos.x() / tab_width;
			
			if (clicked && tab >= 0 && tab < TAB_COUNT)
			{
				set_object_type(static_cast<ObjectType>(tab));
				return Cursor::State::CLICKING;
			}
			
			return Cursor::State::CANCLICK;
		}
		
		// Check entries
		int16_t entry = get_entry_at(cursorpos);
		hovered_index = entry;
		
		if (clicked && entry >= 0)
		{
			selected_index = entry;
			LOG(LOG_INFO, "[ObjectPalette] Selected: " << filtered_objects[entry].name.c_str());
			return Cursor::State::CLICKING;
		}
		
		return entry >= 0 ? Cursor::State::CANCLICK : Cursor::State::IDLE;
	}
	
	void UIObjectPalette::send_scroll(double yoffset)
	{
		if (max_scroll > 0)
		{
			scroll_offset -= static_cast<int16_t>(yoffset);
			scroll_offset = std::max(static_cast<int16_t>(0), 
				std::min(scroll_offset, max_scroll));
		}
	}
	
	int16_t UIObjectPalette::get_entry_at(Point<int16_t> pos) const
	{
		Point<int16_t> relative_pos = pos - position;
		
		if (relative_pos.y() < 40) // Skip tabs
			return -1;
			
		int16_t entry_index = (relative_pos.y() - 40) / ENTRY_HEIGHT;
		
		if (entry_index >= 0 && entry_index < VISIBLE_ENTRIES)
		{
			int16_t actual_index = scroll_offset + entry_index;
			if (actual_index < filtered_objects.size())
				return actual_index;
		}
		
		return -1;
	}
	
	void UIObjectPalette::set_object_type(ObjectType type)
	{
		selected_type = type;
		selected_index = -1;
		scroll_offset = 0;
		
		// Filter objects by type
		filtered_objects.clear();
		for (const auto& obj : objects)
		{
			if (obj.type == type)
				filtered_objects.push_back(obj);
		}
		
		// Update max scroll
		max_scroll = std::max(static_cast<int16_t>(0), 
			static_cast<int16_t>(filtered_objects.size() - VISIBLE_ENTRIES));
	}
	
	const UIObjectPalette::ObjectEntry* UIObjectPalette::get_selected_object() const
	{
		if (selected_index >= 0 && selected_index < filtered_objects.size())
			return &filtered_objects[selected_index];
		return nullptr;
	}
	
	Button::State UIObjectPalette::button_pressed(uint16_t buttonid)
	{
		return Button::State::NORMAL;
	}
}