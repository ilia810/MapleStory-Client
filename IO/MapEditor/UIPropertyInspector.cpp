//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "UIPropertyInspector.h"
#include "../../Graphics/GraphicsGL.h"
#include "../Components/MapleButton.h"
#include "../../MapleStory.h"
#include <algorithm>
#include <iostream>

namespace ms
{
	UIPropertyInspector::PropertyField::PropertyField(const std::string& lbl, Point<int16_t> pos, int16_t width,
		std::function<std::string()> get, std::function<void(const std::string&)> set, bool numeric) :
		label(lbl), position(pos), getter(get), setter(set), numeric_only(numeric)
	{
		field = std::make_unique<Textfield>(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK, 
			Rectangle<int16_t>(pos.x(), pos.x() + width, pos.y(), pos.y() + 20),
			50
		);
		field->change_text(getter());
	}
	
	UIPropertyInspector::UIPropertyInspector() :
		mode(MODE_NONE),
		selected_field(-1),
		current_map(nullptr),
		current_portal(nullptr),
		current_npc(nullptr),
		current_mob(nullptr),
		spawn_x(nullptr),
		spawn_y(nullptr),
		header_text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE),
		label_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK),
		inspector_bg(static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(255)),
		section_bg(static_cast<uint8_t>(250), static_cast<uint8_t>(250), static_cast<uint8_t>(250), static_cast<uint8_t>(255))
	{
		// Position at right side of screen
		position = Point<int16_t>(750, 60);
		dimension = Point<int16_t>(INSPECTOR_WIDTH, 400);
		active = true;
		
		// TODO: Add Apply and Revert buttons once we have proper button resources
		// For now, we'll apply changes immediately when typing
	}
	
	void UIPropertyInspector::draw(float inter) const
	{
		// Draw background
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			dimension.y(),
			inspector_bg.r(),
			inspector_bg.g(),
			inspector_bg.b(),
			inspector_bg.a()
		);
		
		// Draw header
		std::string header_title = "Properties";
		switch (mode)
		{
		case MODE_MAP: header_title = "Map Properties"; break;
		case MODE_PORTAL: header_title = "Portal Properties"; break;
		case MODE_NPC: header_title = "NPC Properties"; break;
		case MODE_MOB: header_title = "Mob Spawn Properties"; break;
		case MODE_SPAWN: header_title = "Spawn Point"; break;
		}
		
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			30,
			static_cast<uint8_t>(100),
			static_cast<uint8_t>(100),
			static_cast<uint8_t>(100),
			static_cast<uint8_t>(255)
		);
		
		Text title_text(Text::Font::A12B, Text::Alignment::CENTER, Color::Name::WHITE);
		title_text.change_text(header_title);
		title_text.draw(DrawArgument(position + Point<int16_t>(dimension.x() / 2, 15)));
		
		// Draw properties
		if (mode != MODE_NONE)
		{
			for (const auto& prop : properties)
			{
				// Draw label
				Text label(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK);
				label.change_text(prop.label + ":");
				label.draw(DrawArgument(position + prop.position - Point<int16_t>(0, 20)));
				
				// Draw field background
				GraphicsGL::get().drawrectangle(
					position.x() + prop.position.x(),
					position.y() + prop.position.y(),
					200, // Field width
					20,  // Field height
					static_cast<uint8_t>(255),
					static_cast<uint8_t>(255),
					static_cast<uint8_t>(255),
					static_cast<uint8_t>(255)
				);
				
				// Draw field
				prop.field->draw(position);
			}
		}
		else
		{
			// Draw "No selection" message
			Text no_selection(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::GRAY);
			no_selection.change_text("No object selected");
			no_selection.draw(DrawArgument(position + Point<int16_t>(dimension.x() / 2, dimension.y() / 2)));
		}
		
		// TODO: Draw buttons when implemented
	}
	
	void UIPropertyInspector::update()
	{
		for (auto& prop : properties)
		{
			prop.field->update();
		}
		
		// TODO: Update buttons when implemented
	}
	
	Cursor::State UIPropertyInspector::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Rectangle<int16_t> bounds(position.x(), position.x() + dimension.x(), 
			position.y(), position.y() + dimension.y());
			
		if (!bounds.contains(cursorpos))
		{
			return UIElement::send_cursor(clicked, cursorpos);
		}
		
		// TODO: Check buttons when implemented
		
		// Check text fields
		for (size_t i = 0; i < properties.size(); i++)
		{
			auto& prop = properties[i];
			Cursor::State state = prop.field->send_cursor(cursorpos - position, clicked);
			
			if (state != Cursor::State::IDLE)
			{
				if (clicked)
				{
					// Apply changes from previously selected field
					if (selected_field >= 0 && selected_field < properties.size())
					{
						apply_field_changes(selected_field);
					}
					
					selected_field = static_cast<int16_t>(i);
					
					// Deselect other fields
					for (size_t j = 0; j < properties.size(); j++)
					{
						if (j != i)
							properties[j].field->set_state(Textfield::State::NORMAL);
					}
				}
				return state;
			}
		}
		
		return Cursor::State::IDLE;
	}
	
	void UIPropertyInspector::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (selected_field >= 0 && selected_field < properties.size())
		{
			properties[selected_field].field->send_key(KeyType::Id::TEXT, keycode, pressed);
			
			// Tab to next field
			if (pressed && keycode == 9) // Tab key
			{
				properties[selected_field].field->set_state(Textfield::State::NORMAL);
				selected_field = (selected_field + 1) % properties.size();
				properties[selected_field].field->set_state(Textfield::State::FOCUSED);
			}
		}
	}
	
	Button::State UIPropertyInspector::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_APPLY:
			apply_changes();
			if (on_change)
				on_change();
			break;
			
		case BT_REVERT:
			update_field_values();
			break;
		}
		
		return Button::State::NORMAL;
	}
	
	void UIPropertyInspector::set_map_properties(MapFormat* map)
	{
		mode = MODE_MAP;
		current_map = map;
		properties.clear();
		selected_field = -1;
		
		if (!map) return;
		
		int16_t y_offset = 50;
		
		// Map name
		properties.emplace_back("Map Name", Point<int16_t>(20, y_offset), 250,
			[this]() { return current_map->get_name(); },
			[this](const std::string& val) { current_map->set_name(val); }
		);
		y_offset += FIELD_SPACING;
		
		// Map size
		properties.emplace_back("Width", Point<int16_t>(20, y_offset), 100,
			[this]() { return std::to_string(current_map->get_width()); },
			[this](const std::string& val) { 
				int16_t width = static_cast<int16_t>(std::stoi(val));
				current_map->set_size(width, current_map->get_height());
			},
			true
		);
		
		properties.emplace_back("Height", Point<int16_t>(140, y_offset), 100,
			[this]() { return std::to_string(current_map->get_height()); },
			[this](const std::string& val) { 
				int16_t height = static_cast<int16_t>(std::stoi(val));
				current_map->set_size(current_map->get_width(), height);
			},
			true
		);
		y_offset += FIELD_SPACING;
		
		// Return map
		properties.emplace_back("Return Map ID", Point<int16_t>(20, y_offset), 150,
			[this]() { return std::to_string(current_map->get_return_map()); },
			[this](const std::string& val) { current_map->set_return_map(std::stoi(val)); },
			true
		);
		
		update_field_values();
	}
	
	void UIPropertyInspector::set_portal(PortalData* portal)
	{
		mode = MODE_PORTAL;
		current_portal = portal;
		properties.clear();
		selected_field = -1;
		
		if (!portal) return;
		
		int16_t y_offset = 50;
		
		// Portal position
		properties.emplace_back("X Position", Point<int16_t>(20, y_offset), 100,
			[this]() { return std::to_string(current_portal->x); },
			[this](const std::string& val) { current_portal->x = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		
		properties.emplace_back("Y Position", Point<int16_t>(140, y_offset), 100,
			[this]() { return std::to_string(current_portal->y); },
			[this](const std::string& val) { current_portal->y = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		y_offset += FIELD_SPACING;
		
		// Portal type
		properties.emplace_back("Portal Type", Point<int16_t>(20, y_offset), 150,
			[this]() { return current_portal->type; },
			[this](const std::string& val) { current_portal->type = val; }
		);
		y_offset += FIELD_SPACING;
		
		// Destination
		properties.emplace_back("Destination Map", Point<int16_t>(20, y_offset), 150,
			[this]() { return current_portal->destination_map; },
			[this](const std::string& val) { current_portal->destination_map = val; }
		);
		y_offset += FIELD_SPACING;
		
		properties.emplace_back("Destination ID", Point<int16_t>(20, y_offset), 150,
			[this]() { return std::to_string(current_portal->destination_id); },
			[this](const std::string& val) { current_portal->destination_id = std::stoi(val); },
			true
		);
		
		update_field_values();
	}
	
	void UIPropertyInspector::set_npc(NpcData* npc)
	{
		mode = MODE_NPC;
		current_npc = npc;
		properties.clear();
		selected_field = -1;
		
		if (!npc) return;
		
		int16_t y_offset = 50;
		
		// NPC ID
		properties.emplace_back("NPC ID", Point<int16_t>(20, y_offset), 150,
			[this]() { return std::to_string(current_npc->id); },
			[this](const std::string& val) { current_npc->id = std::stoi(val); },
			true
		);
		y_offset += FIELD_SPACING;
		
		// Position
		properties.emplace_back("X Position", Point<int16_t>(20, y_offset), 100,
			[this]() { return std::to_string(current_npc->x); },
			[this](const std::string& val) { current_npc->x = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		
		properties.emplace_back("Y Position", Point<int16_t>(140, y_offset), 100,
			[this]() { return std::to_string(current_npc->y); },
			[this](const std::string& val) { current_npc->y = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		y_offset += FIELD_SPACING;
		
		// Flip
		properties.emplace_back("Flip (0/1)", Point<int16_t>(20, y_offset), 50,
			[this]() { return current_npc->flip ? "1" : "0"; },
			[this](const std::string& val) { current_npc->flip = (val == "1"); },
			true
		);
		
		update_field_values();
	}
	
	void UIPropertyInspector::set_mob_spawn(MobSpawnData* mob)
	{
		mode = MODE_MOB;
		current_mob = mob;
		properties.clear();
		selected_field = -1;
		
		if (!mob) return;
		
		int16_t y_offset = 50;
		
		// Mob ID
		properties.emplace_back("Mob ID", Point<int16_t>(20, y_offset), 150,
			[this]() { return std::to_string(current_mob->id); },
			[this](const std::string& val) { current_mob->id = std::stoi(val); },
			true
		);
		y_offset += FIELD_SPACING;
		
		// Position
		properties.emplace_back("X Position", Point<int16_t>(20, y_offset), 100,
			[this]() { return std::to_string(current_mob->x); },
			[this](const std::string& val) { current_mob->x = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		
		properties.emplace_back("Y Position", Point<int16_t>(140, y_offset), 100,
			[this]() { return std::to_string(current_mob->y); },
			[this](const std::string& val) { current_mob->y = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		y_offset += FIELD_SPACING;
		
		// Respawn time
		properties.emplace_back("Respawn Time (sec)", Point<int16_t>(20, y_offset), 150,
			[this]() { return std::to_string(current_mob->respawn_time); },
			[this](const std::string& val) { current_mob->respawn_time = std::stoi(val); },
			true
		);
		
		update_field_values();
	}
	
	void UIPropertyInspector::set_spawn_point(int16_t* x, int16_t* y)
	{
		mode = MODE_SPAWN;
		spawn_x = x;
		spawn_y = y;
		properties.clear();
		selected_field = -1;
		
		if (!x || !y) return;
		
		int16_t y_offset = 50;
		
		// Spawn position
		properties.emplace_back("X Position", Point<int16_t>(20, y_offset), 100,
			[this]() { return std::to_string(*spawn_x); },
			[this](const std::string& val) { *spawn_x = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		
		properties.emplace_back("Y Position", Point<int16_t>(140, y_offset), 100,
			[this]() { return std::to_string(*spawn_y); },
			[this](const std::string& val) { *spawn_y = static_cast<int16_t>(std::stoi(val)); },
			true
		);
		
		update_field_values();
	}
	
	void UIPropertyInspector::clear_selection()
	{
		mode = MODE_NONE;
		properties.clear();
		selected_field = -1;
		current_map = nullptr;
		current_portal = nullptr;
		current_npc = nullptr;
		current_mob = nullptr;
		spawn_x = nullptr;
		spawn_y = nullptr;
	}
	
	void UIPropertyInspector::update_field_values()
	{
		for (auto& prop : properties)
		{
			prop.field->change_text(prop.getter());
		}
	}
	
	void UIPropertyInspector::apply_changes()
	{
		for (auto& prop : properties)
		{
			std::string value = prop.field->get_text();
			
			// Validate numeric fields
			if (prop.numeric_only)
			{
				try
				{
					std::stoi(value);
					prop.setter(value);
				}
				catch (...)
				{
					// Invalid number, revert to original
					LOG(LOG_INFO, "[PropertyInspector] Invalid number for property: " << prop.label);
					prop.field->change_text(prop.getter());
				}
			}
			else
			{
				prop.setter(value);
			}
		}
		
		LOG(LOG_INFO, "[PropertyInspector] Changes applied");
	}
	
	void UIPropertyInspector::apply_field_changes(int16_t field_index)
	{
		if (field_index < 0 || field_index >= properties.size())
			return;
			
		auto& prop = properties[field_index];
		std::string value = prop.field->get_text();
		
		// Validate numeric fields
		if (prop.numeric_only)
		{
			try
			{
				std::stoi(value);
				prop.setter(value);
			}
			catch (...)
			{
				// Invalid number, revert to original
				LOG(LOG_INFO, "[PropertyInspector] Invalid number for property: " << prop.label);
				prop.field->change_text(prop.getter());
			}
		}
		else
		{
			prop.setter(value);
		}
		
		// Notify change callback
		if (on_change)
			on_change();
	}
}