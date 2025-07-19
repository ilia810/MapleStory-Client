//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "UIStateMapEditor.h"

#include "UI.h"
#include "Window.h"
#include "UITypes/UIStatusBar.h"
#include "UITypes/UIMiniMap.h"
#include "UITypes/UIBuffList.h"
#include "UITypes/UIKeyConfig.h"

#include "../Gameplay/Stage.h"
#include "../Character/Player.h"
#include "../Character/CharStats.h"
#include "../Net/Login.h"
#include "../Configuration.h"
#include "MapEditor/MapFormat.h"
#include "MapEditor/MapFormatParser.h"
#include "MapEditor/MapRenderer.h"
#include "MapEditor/UITilePalette.h"
#include "MapEditor/UIMapEditorToolbar.h"
#include "MapEditor/UIObjectPalette.h"
#include "MapEditor/UIPropertyInspector.h"
#include "MapEditor/UIAssetBrowser.h"

namespace ms
{
	UIStateMapEditor::UIStateMapEditor() : 
		elements(), 
		elementorder(), 
		keyboard(), 
		tooltip(), 
		dragged_icon(),
		current_map(nullptr),
		map_renderer(std::make_unique<MapRenderer>()),
		mouse_position(0, 0),
		is_placing_tile(false),
		is_dragging(false),
		last_tile_position(-1, -1),
		selection_type(SEL_NONE),
		selected_index(-1),
		editor_mode(MODE_EDIT),
		player_position(0, 0)
	{
		LOG(LOG_INFO, "[MapEditor] Initializing map editor state");
		
		// Initialize focused element
		// Note: UIStateMapEditor doesn't have a 'focused' member like UIStateLogin
		
		init_editor_ui();
		// Load a test map using the new format parser
		load_test_map();
		LOG(LOG_INFO, "[MapEditor] Map editor state initialized");
	}

	void UIStateMapEditor::init_editor_ui()
	{
		LOG(LOG_INFO, "[MapEditor] Starting UI initialization");
		
		// Create map editor UI elements
		try {
			// Create toolbar first (appears at top)
			emplace<UIMapEditorToolbar>();
			LOG(LOG_INFO, "[MapEditor] Toolbar created successfully");
			
			// Set up toolbar callbacks
			if (auto* toolbar = dynamic_cast<UIMapEditorToolbar*>(get(UIElement::Type::MAP_EDITOR_TOOLBAR)))
			{
				toolbar->set_new_callback([this]() { new_map(); });
				toolbar->set_open_callback([this]() { open_map(); });
				toolbar->set_save_callback([this]() { save_map(); });
				toolbar->set_save_as_callback([this]() { save_map_as(); });
				toolbar->set_playtest_callback([this]() { 
					if (editor_mode == MODE_EDIT) 
						switch_to_play_mode(); 
					else 
						switch_to_edit_mode(); 
				});
				toolbar->set_asset_filter_callback([this](const std::string& filter) {
					if (auto* asset_browser = dynamic_cast<UIAssetBrowser*>(get(UIElement::Type::MAP_EDITOR_ASSET_BROWSER)))
					{
						asset_browser->set_filter(filter);
					}
				});
			}
			
			// Create tile palette
			emplace<UITilePalette>();
			LOG(LOG_INFO, "[MapEditor] Tile palette created successfully");
			
			// Create object palette
			emplace<UIObjectPalette>();
			LOG(LOG_INFO, "[MapEditor] Object palette created successfully");
			
			// Create property inspector
			emplace<UIPropertyInspector>();
			LOG(LOG_INFO, "[MapEditor] Property inspector created successfully");
			
			// Create asset browser
			emplace<UIAssetBrowser>();
			LOG(LOG_INFO, "[MapEditor] Asset browser created successfully");
			
			// Set up asset browser callback
			if (auto* asset_browser = dynamic_cast<UIAssetBrowser*>(get(UIElement::Type::MAP_EDITOR_ASSET_BROWSER)))
			{
				asset_browser->set_asset_selection_callback([this](int32_t asset_id) {
					add_asset_to_palette(asset_id);
				});
			}
			
			// Set the palette if map is loaded
			if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
			{
				if (current_map)
				{
					palette_ui->set_palette(&current_map->get_palette());
				}
			}
		}
		catch (const std::exception& e) {
			LOG(LOG_ERROR, "[MapEditor] Failed to create UI elements: " << e.what());
		}
		catch (...) {
			LOG(LOG_ERROR, "[MapEditor] Failed to create UI elements: Unknown error");
		}
		
		// Temporarily skip other UI element creation to isolate crash
		/*
		try {
			// Create dummy StatsEntry for UI elements that need it
			StatsEntry dummy_entry;
			dummy_entry.name = "MapEditor";
			dummy_entry.female = false;
			dummy_entry.exp = 0;
			dummy_entry.mapid = 100000000;
			dummy_entry.portal = 0;
			dummy_entry.rank = std::make_pair(0, 0);
			dummy_entry.jobrank = std::make_pair(0, 0);
			
			// Initialize some basic stats to prevent crashes
			dummy_entry.stats[MapleStat::Id::LEVEL] = 1;
			dummy_entry.stats[MapleStat::Id::HP] = 100;
			dummy_entry.stats[MapleStat::Id::MAXHP] = 100;
			dummy_entry.stats[MapleStat::Id::MP] = 100;
			dummy_entry.stats[MapleStat::Id::MAXMP] = 100;
			
			CharStats dummy_stats(dummy_entry);
			
			// Create basic UI elements for the editor
			emplace<UIStatusBar>(dummy_stats);
			emplace<UIMiniMap>(dummy_stats);
			emplace<UIBuffList>();
		}
		catch (const std::exception& e) {
			LOG(LOG_ERROR, "[MapEditor] Failed to initialize UI: " << e.what());
		}
		catch (...) {
			LOG(LOG_ERROR, "[MapEditor] Failed to initialize UI: Unknown error");
		}
		*/
		
		// TODO: Add editor-specific UI elements like:
		// - Tile palette
		// - Object palette
		// - Property inspector
		// - File menu (New, Open, Save)
		
		LOG(LOG_INFO, "[MapEditor] UI initialized");
	}

	void UIStateMapEditor::load_default_map()
	{
		// Load a default empty map or the last edited map
		// For now, let's load Henesys (map id 100000000) as a starting point
		int32_t default_map_id = 100000000;
		
		LOG(LOG_INFO, "[MapEditor] Loading default map: " << default_map_id);
		
		try {
			// Skip player creation if we're in editor mode - just load the map directly
			Stage::get().load(default_map_id, 0);
		}
		catch (const std::exception& e) {
			LOG(LOG_ERROR, "[MapEditor] Failed to load map: " << e.what());
			// Continue without map - we'll add a blank map creation later
			return;
		}
		catch (...) {
			LOG(LOG_ERROR, "[MapEditor] Failed to load map: Unknown error");
			// Continue without map - we'll add a blank map creation later
			return;
		}
		
		// Create a minimal player object for navigation (needed by Stage)
		CharEntry dummy_entry;
		
		// Initialize stats
		dummy_entry.stats.name = "MapEditor";
		dummy_entry.stats.female = false;
		dummy_entry.stats.exp = 0;
		dummy_entry.stats.mapid = default_map_id;
		dummy_entry.stats.portal = 0;
		dummy_entry.stats.rank = std::make_pair(0, 0);
		dummy_entry.stats.jobrank = std::make_pair(0, 0);
		
		// Initialize base stats
		dummy_entry.stats.stats[MapleStat::Id::LEVEL] = 1;
		dummy_entry.stats.stats[MapleStat::Id::HP] = 100;
		dummy_entry.stats.stats[MapleStat::Id::MAXHP] = 100;
		dummy_entry.stats.stats[MapleStat::Id::MP] = 100;
		dummy_entry.stats.stats[MapleStat::Id::MAXMP] = 100;
		dummy_entry.stats.stats[MapleStat::Id::STR] = 4;
		dummy_entry.stats.stats[MapleStat::Id::DEX] = 4;
		dummy_entry.stats.stats[MapleStat::Id::INT] = 4;
		dummy_entry.stats.stats[MapleStat::Id::LUK] = 4;
		
		// Initialize look
		dummy_entry.look.female = false;
		dummy_entry.look.skin = 0;
		dummy_entry.look.faceid = 20000; // Default face
		dummy_entry.look.hairid = 30000; // Default hair
		
		// Initialize character id
		dummy_entry.id = 1; // Dummy character ID
		
		Stage::get().loadplayer(dummy_entry);
	}

	void UIStateMapEditor::draw(float inter, Point<int16_t> cursor) const
	{
		// Draw the map first as background
		if (map_renderer && current_map)
		{
			// Check grid visibility from toolbar (only in edit mode)
			if (editor_mode == MODE_EDIT)
			{
				if (auto* toolbar = dynamic_cast<UIMapEditorToolbar*>(const_cast<UIStateMapEditor*>(this)->get(UIElement::Type::MAP_EDITOR_TOOLBAR)))
				{
					map_renderer->set_grid_visible(toolbar->is_grid_visible());
				}
			}
			else
			{
				// Hide grid in play mode
				map_renderer->set_grid_visible(false);
			}
			
			map_renderer->render(Point<int16_t>(100, 100), 1.0f);
		}
		
		// Draw player in play mode
		if (editor_mode == MODE_PLAY)
		{
			draw_player();
		}
		
		// Only draw UI elements in edit mode
		if (editor_mode == MODE_EDIT)
		{
			for (auto& type : elementorder)
			{
				auto& element = elements[type];
				if (element && element->is_active())
					element->draw(inter);
			}

			if (tooltip)
				tooltip->draw(cursor);

			if (dragged_icon)
				dragged_icon->dragdraw(cursor);
		}
	}

	void UIStateMapEditor::update()
	{
		// Update player movement in play mode
		if (editor_mode == MODE_PLAY)
		{
			update_player_movement();
		}
		
		// Only update UI elements in edit mode
		if (editor_mode == MODE_EDIT)
		{
			for (auto& type : elementorder)
			{
				auto& element = elements[type];
				if (element && element->is_active())
					element->update();
			}
		}
	}

	void UIStateMapEditor::doubleclick(Point<int16_t> pos)
	{
		// Disable double-click in play mode
		if (editor_mode == MODE_PLAY)
			return;
			
		if (auto* front = get_front(pos))
			front->doubleclick(pos);
		
		// Place tile on map
		if (current_map && map_renderer)
		{
			Point<int16_t> tile_pos = map_renderer->screen_to_tile(pos - Point<int16_t>(100, 100));
			if (tile_pos.x() >= 0 && tile_pos.y() >= 0)
			{
				if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
				{
					char selected_tile = palette_ui->get_selected_tile();
					current_map->set_tile(tile_pos.x(), tile_pos.y(), selected_tile);
					LOG(LOG_INFO, "[MapEditor] Placed tile '" << selected_tile << "' at (" << tile_pos.x() << ", " << tile_pos.y() << ")");
				}
			}
		}
	}

	void UIStateMapEditor::rightclick(Point<int16_t> pos)
	{
		// Disable right-click in play mode
		if (editor_mode == MODE_PLAY)
			return;
			
		if (auto* front = get_front(pos))
			front->rightclick(pos);
		
		// Cancel tile placement on right click
		is_placing_tile = false;
		is_dragging = false;
	}

	void UIStateMapEditor::send_key(KeyType::Id type, int32_t action, bool down, bool escape)
	{
		// Handle editor-specific keys
		if (down)
		{
			if (escape)
			{
				// In play mode, ESC returns to edit mode
				if (editor_mode == MODE_PLAY)
				{
					switch_to_edit_mode();
					return;
				}
				// TODO: Show editor menu or exit confirmation for edit mode
			}
			else if (action == 261) // GLFW_KEY_DELETE
			{
				// Delete selected object (only in edit mode)
				if (editor_mode == MODE_EDIT)
				{
					delete_selected_object();
				}
			}
			else if (action == 300) // GLFW_KEY_F11
			{
				// Toggle maximize window
				Window::get().maximize_window();
			}
			// Player movement in play mode
			else if (editor_mode == MODE_PLAY)
			{
				handle_play_mode_input(action, true);
			}
		}
		else if (!down && editor_mode == MODE_PLAY)
		{
			// Handle key release for smooth movement
			handle_play_mode_input(action, false);
		}
		
		// Handle text input if any UI element is focused (only in edit mode)
		if (editor_mode == MODE_EDIT)
		{
			for (auto& type_elem : elementorder)
			{
				auto& element = elements[type_elem];
				if (element && element->is_active())
				{
					element->send_key(action, down, escape);
					return;
				}
			}
		}
	}

	Cursor::State UIStateMapEditor::send_cursor(Point<int16_t> cursor_position, Cursor::State cursor_state)
	{
		mouse_position = cursor_position;
		
		// Disable cursor interaction in play mode
		if (editor_mode == MODE_PLAY)
		{
			return Cursor::State::IDLE;
		}
		
		if (tooltip)
		{
			clear_tooltip(Tooltip::Parent::NONE);
		}

		if (dragged_icon)
		{
			switch (cursor_state)
			{
			case Cursor::State::CLICKING:
				// Handle drop
				dragged_icon = {};
				return Cursor::State::IDLE;
			default:
				return Cursor::State::GRABBING;
			}
		}
		else
		{
			UIElement* front = nullptr;
			UIElement::Type front_type = UIElement::Type::NONE;

			for (auto& type : elementorder)
			{
				auto& element = elements[type];
				if (element && element->is_active() && element->is_in_range(cursor_position))
				{
					front = element.get();
					front_type = type;
				}
			}

			if (front)
			{
				// Let UI elements handle the cursor first
				Cursor::State ui_state = front->send_cursor(cursor_state == Cursor::State::CLICKING, cursor_position);
				
				// If UI handled it, return that state
				if (ui_state != Cursor::State::IDLE)
				{
					is_dragging = false;
					return ui_state;
				}
			}
			
			// Handle map painting if no UI element is active
			if (current_map && map_renderer)
			{
				// Get the toolbar to check selected tool
				auto* toolbar = dynamic_cast<UIMapEditorToolbar*>(get(UIElement::Type::MAP_EDITOR_TOOLBAR));
				if (!toolbar)
					return Cursor::State::IDLE;
					
				auto selected_tool = toolbar->get_selected_tool();
				
				// Handle different tools
				if (selected_tool == UIMapEditorToolbar::TB_PAINT || selected_tool == UIMapEditorToolbar::TB_ERASE)
				{
					// Tile painting/erasing
					Point<int16_t> tile_pos = map_renderer->screen_to_tile(cursor_position - Point<int16_t>(100, 100));
					
					if (cursor_state == Cursor::State::CLICKING)
					{
						// Start dragging
						is_dragging = true;
						last_tile_position = tile_pos;
						
						// Place/erase initial tile
						if (tile_pos.x() >= 0 && tile_pos.y() >= 0 && 
							tile_pos.x() < current_map->get_width() && tile_pos.y() < current_map->get_height())
						{
							if (selected_tool == UIMapEditorToolbar::TB_PAINT)
							{
								if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
								{
									char selected_tile = palette_ui->get_selected_tile();
									current_map->set_tile(tile_pos.x(), tile_pos.y(), selected_tile);
								}
							}
							else if (selected_tool == UIMapEditorToolbar::TB_ERASE)
							{
								current_map->set_tile(tile_pos.x(), tile_pos.y(), ' ');
							}
						}
						
						return Cursor::State::CLICKING;
					}
					else if (is_dragging)
					{
						// Continue dragging - paint/erase tiles as we move
						if (tile_pos != last_tile_position && 
							tile_pos.x() >= 0 && tile_pos.y() >= 0 &&
							tile_pos.x() < current_map->get_width() && tile_pos.y() < current_map->get_height())
						{
							if (selected_tool == UIMapEditorToolbar::TB_PAINT)
							{
								if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
								{
									char selected_tile = palette_ui->get_selected_tile();
									current_map->set_tile(tile_pos.x(), tile_pos.y(), selected_tile);
								}
							}
							else if (selected_tool == UIMapEditorToolbar::TB_ERASE)
							{
								current_map->set_tile(tile_pos.x(), tile_pos.y(), ' ');
							}
							
							last_tile_position = tile_pos;
						}
						
						return Cursor::State::CLICKING;
					}
					else
					{
						// Not clicking, stop dragging
						is_dragging = false;
						return Cursor::State::CANCLICK;
					}
				}
				else if (selected_tool == UIMapEditorToolbar::TB_SELECT)
				{
					Point<int16_t> tile_pos = map_renderer->screen_to_tile(cursor_position - Point<int16_t>(100, 100));
					
					// Check if we're placing a new object
					auto* obj_palette = dynamic_cast<UIObjectPalette*>(get(UIElement::Type::MAP_EDITOR_OBJECT_PALETTE));
					if (obj_palette && obj_palette->get_selected_object())
					{
						// Object placement mode
						if (cursor_state == Cursor::State::CLICKING)
						{
							// Place object at clicked position
							if (tile_pos.x() >= 0 && tile_pos.y() >= 0 &&
								tile_pos.x() < current_map->get_width() && tile_pos.y() < current_map->get_height())
							{
								const auto* selected_obj = obj_palette->get_selected_object();
								
								switch (selected_obj->type)
								{
								case UIObjectPalette::OBJ_PORTAL:
									{
										PortalData portal;
										portal.x = tile_pos.x();
										portal.y = tile_pos.y();
										portal.type = selected_obj->id == 1 ? "hidden" : "regular";
										portal.destination_map = "100000000"; // Default to Henesys
										portal.destination_id = 0;
										current_map->add_portal(portal);
										LOG(LOG_INFO, "[MapEditor] Placed portal at (" << tile_pos.x() << ", " << tile_pos.y() << ")");
									}
									break;
									
								case UIObjectPalette::OBJ_NPC:
									{
										NpcData npc;
										npc.id = selected_obj->id;
										npc.x = tile_pos.x();
										npc.y = tile_pos.y();
										current_map->add_npc(npc);
										LOG(LOG_INFO, "[MapEditor] Placed NPC " << selected_obj->id << " at (" << tile_pos.x() << ", " << tile_pos.y() << ")");
									}
									break;
									
								case UIObjectPalette::OBJ_MOB:
									{
										MobSpawnData mob;
										mob.id = selected_obj->id;
										mob.x = tile_pos.x();
										mob.y = tile_pos.y();
										mob.respawn_time = 30; // Default 30 second respawn
										current_map->add_mob_spawn(mob);
										LOG(LOG_INFO, "[MapEditor] Placed mob spawn " << selected_obj->id << " at (" << tile_pos.x() << ", " << tile_pos.y() << ")");
									}
									break;
									
								case UIObjectPalette::OBJ_SPAWN:
									if (selected_obj->id == 0) // Player spawn
									{
										current_map->set_player_spawn(tile_pos.x(), tile_pos.y());
										LOG(LOG_INFO, "[MapEditor] Set player spawn at (" << tile_pos.x() << ", " << tile_pos.y() << ")");
									}
									break;
								}
								
								return Cursor::State::CLICKING;
							}
						}
						
						return Cursor::State::CANCLICK;
					}
					else
					{
						// Selection mode - click to select objects
						if (cursor_state == Cursor::State::CLICKING)
						{
							select_object_at(tile_pos);
							return Cursor::State::CLICKING;
						}
						
						return Cursor::State::CANCLICK;
					}
				}
			}
			
			return Cursor::State::IDLE;
		}
	}

	void UIStateMapEditor::send_scroll(double yoffset)
	{
		// TODO: Handle zoom in/out for map editor
	}

	void UIStateMapEditor::send_close()
	{
		// TODO: Ask for save confirmation before closing
		UI::get().quit();
	}

	void UIStateMapEditor::drag_icon(Icon* icon)
	{
		dragged_icon = icon;
	}

	void UIStateMapEditor::clear_tooltip(Tooltip::Parent parent)
	{
		if (parent == Tooltip::Parent::NONE || tooltip)
		{
			tooltip = {};
		}
	}

	void UIStateMapEditor::show_equip(Tooltip::Parent parent, int16_t slot)
	{
		// Not needed for map editor
	}

	void UIStateMapEditor::show_item(Tooltip::Parent parent, int32_t itemid)
	{
		// Not needed for map editor  
	}

	void UIStateMapEditor::show_skill(Tooltip::Parent parent, int32_t skill_id, int32_t level, int32_t masterlevel, int64_t expiration)
	{
		// Not needed for map editor
	}

	void UIStateMapEditor::show_text(Tooltip::Parent parent, std::string text)
	{
		// TODO: Implement text tooltips for map editor
	}

	void UIStateMapEditor::show_map(Tooltip::Parent parent, std::string name, std::string description, int32_t mapid, bool bolded, bool portal)
	{
		// Could be used to show map info in editor
		// TODO: Implement map tooltips for map editor
	}

	UIStateMapEditor::Iterator UIStateMapEditor::pre_add(UIElement::Type type, bool toggled, bool focused)
	{
		auto& element = elements[type];
		
		if (element)
		{
			element->toggle_active();
			return { nullptr, UIElement::Type::NONE };
		}

		if (focused)
		{
			elementorder.push_back(type);
		}
		else
		{
			auto begin = elementorder.begin();
			elementorder.insert(begin, type);
		}

		return elements.find(type);
	}

	void UIStateMapEditor::remove(UIElement::Type type)
	{
		if (type == UIElement::Type::NONE)
			return;

		elementorder.remove(type);
		
		if (dragged_icon)
			dragged_icon = {};

		elements[type].release();
	}

	UIElement* UIStateMapEditor::get(UIElement::Type type)
	{
		return elements[type].get();
	}

	UIElement* UIStateMapEditor::get_front(std::list<UIElement::Type> types)
	{
		for (auto& type : types)
		{
			auto& element = elements[type];
			if (element && element->is_active())
				return element.get();
		}
		return nullptr;
	}

	UIElement* UIStateMapEditor::get_front(Point<int16_t> pos)
	{
		for (auto it = elementorder.rbegin(); it != elementorder.rend(); ++it)
		{
			auto& element = elements[*it];
			if (element && element->is_active() && element->is_in_range(pos))
				return element.get();
		}
		return nullptr;
	}
	
	void UIStateMapEditor::load_test_map()
	{
		LOG(LOG_INFO, "[MapEditor] Loading test map");
		
		MapFormatParser parser;
		auto map_data = parser.parse_file("maps/test_map.yml");
		
		if (map_data)
		{
			LOG(LOG_INFO, "[MapEditor] Successfully loaded test map: " << map_data->get_name());
			LOG(LOG_INFO, "[MapEditor] Map size: " << map_data->get_width() << "x" << map_data->get_height());
			LOG(LOG_INFO, "[MapEditor] Palette entries: " << map_data->get_palette().size());
			
			// Set the map for rendering
			current_map = std::move(map_data);
			map_renderer->set_map(current_map);
			map_renderer->set_tile_size(40);  // Larger tiles for visibility
			
			// Update tile palette
			if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
			{
				palette_ui->set_palette(&current_map->get_palette());
			}
		}
		else
		{
			LOG(LOG_ERROR, "[MapEditor] Failed to load test map: " << parser.get_error());
		}
	}
	
	void UIStateMapEditor::select_object_at(Point<int16_t> map_pos)
	{
		if (!current_map) return;
		
		clear_selection();
		
		auto* inspector = dynamic_cast<UIPropertyInspector*>(get(UIElement::Type::MAP_EDITOR_PROPERTIES));
		if (!inspector) return;
		
		// Check for objects at this position (with some tolerance)
		const int16_t tolerance = 20; // Pixel tolerance for selection
		
		// Check portals
		auto& portals = const_cast<std::vector<PortalData>&>(current_map->get_portals());
		for (size_t i = 0; i < portals.size(); i++)
		{
			if (std::abs(portals[i].x - map_pos.x()) <= tolerance &&
				std::abs(portals[i].y - map_pos.y()) <= tolerance)
			{
				selection_type = SEL_PORTAL;
				selected_index = static_cast<int32_t>(i);
				inspector->set_portal(&portals[i]);
				LOG(LOG_INFO, "[MapEditor] Selected portal at (" << portals[i].x << ", " << portals[i].y << ")");
				return;
			}
		}
		
		// Check NPCs
		auto& npcs = const_cast<std::vector<NpcData>&>(current_map->get_npcs());
		for (size_t i = 0; i < npcs.size(); i++)
		{
			if (std::abs(npcs[i].x - map_pos.x()) <= tolerance &&
				std::abs(npcs[i].y - map_pos.y()) <= tolerance)
			{
				selection_type = SEL_NPC;
				selected_index = static_cast<int32_t>(i);
				inspector->set_npc(&npcs[i]);
				LOG(LOG_INFO, "[MapEditor] Selected NPC " << npcs[i].id << " at (" << npcs[i].x << ", " << npcs[i].y << ")");
				return;
			}
		}
		
		// Check mob spawns
		auto& mobs = const_cast<std::vector<MobSpawnData>&>(current_map->get_mob_spawns());
		for (size_t i = 0; i < mobs.size(); i++)
		{
			if (std::abs(mobs[i].x - map_pos.x()) <= tolerance &&
				std::abs(mobs[i].y - map_pos.y()) <= tolerance)
			{
				selection_type = SEL_MOB;
				selected_index = static_cast<int32_t>(i);
				inspector->set_mob_spawn(&mobs[i]);
				LOG(LOG_INFO, "[MapEditor] Selected mob spawn " << mobs[i].id << " at (" << mobs[i].x << ", " << mobs[i].y << ")");
				return;
			}
		}
		
		// Check player spawn
		if (std::abs(current_map->get_player_spawn_x() - map_pos.x()) <= tolerance &&
			std::abs(current_map->get_player_spawn_y() - map_pos.y()) <= tolerance)
		{
			selection_type = SEL_SPAWN;
			selected_index = 0;
			// Get mutable references for the inspector
			inspector->set_spawn_point(
				&current_map->get_mutable_player_spawn_x(),
				&current_map->get_mutable_player_spawn_y()
			);
			LOG(LOG_INFO, "[MapEditor] Selected player spawn at (" << 
				current_map->get_player_spawn_x() << ", " << current_map->get_player_spawn_y() << ")");
			return;
		}
		
		// Nothing selected
		LOG(LOG_INFO, "[MapEditor] No object found at (" << map_pos.x() << ", " << map_pos.y() << ")");
	}
	
	void UIStateMapEditor::clear_selection()
	{
		selection_type = SEL_NONE;
		selected_index = -1;
		
		if (auto* inspector = dynamic_cast<UIPropertyInspector*>(get(UIElement::Type::MAP_EDITOR_PROPERTIES)))
		{
			inspector->clear_selection();
		}
	}
	
	void UIStateMapEditor::delete_selected_object()
	{
		if (!current_map || selection_type == SEL_NONE) return;
		
		// TODO: Implement deletion for each object type
		// This would require adding remove methods to MapFormat
		
		clear_selection();
	}
	
	void UIStateMapEditor::new_map()
	{
		LOG(LOG_INFO, "[MapEditor] Creating new map");
		
		// Create a new empty map
		current_map = std::make_shared<MapFormat>();
		current_map->set_name("New Map");
		current_map->set_size(50, 30); // Default size
		current_map->set_return_map(100000000); // Henesys
		current_map->set_player_spawn(25, 15); // Center of map
		
		// Add some default palette entries
		current_map->add_palette_entry('G', PaletteEntry(100450, "Grass_mid"));
		current_map->add_palette_entry('S', PaletteEntry(100451, "Stone"));
		current_map->add_palette_entry('D', PaletteEntry(100452, "Dirt"));
		current_map->add_palette_entry('W', PaletteEntry(100453, "Water"));
		
		// Initialize grid with empty tiles
		current_map->clear_tiles();
		
		// Clear the current file path
		current_map_path.clear();
		
		// Update renderer
		if (map_renderer)
		{
			map_renderer->set_map(current_map);
		}
		
		// Update tile palette
		if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
		{
			palette_ui->set_palette(&current_map->get_palette());
		}
		
		// Clear selection
		clear_selection();
		
		LOG(LOG_INFO, "[MapEditor] New map created");
	}
	
	void UIStateMapEditor::open_map()
	{
		LOG(LOG_INFO, "[MapEditor] Opening map file");
		
		// For now, we'll use a simple text input approach
		// In a real implementation, you would use a file dialog
		
		// TODO: Implement file dialog or text input for file path
		// For testing, let's try to load a map from a fixed location
		std::string test_path = "maps/test_map.yaml";
		
		MapFormatParser parser;
		auto loaded_map = parser.parse_file(test_path);
		
		if (loaded_map)
		{
			current_map = std::move(loaded_map);
			current_map_path = test_path;
			
			// Update renderer
			if (map_renderer)
			{
				map_renderer->set_map(current_map);
			}
			
			// Update tile palette
			if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
			{
				palette_ui->set_palette(&current_map->get_palette());
			}
			
			// Clear selection
			clear_selection();
			
			LOG(LOG_INFO, "[MapEditor] Map loaded successfully from: " << test_path);
		}
		else
		{
			LOG(LOG_ERROR, "[MapEditor] Failed to load map: " << parser.get_error());
		}
	}
	
	void UIStateMapEditor::save_map()
	{
		if (!current_map)
		{
			LOG(LOG_ERROR, "[MapEditor] No map to save");
			return;
		}
		
		if (current_map_path.empty())
		{
			// No current path, use save as
			save_map_as();
			return;
		}
		
		LOG(LOG_INFO, "[MapEditor] Saving map to: " << current_map_path);
		
		MapFormatParser parser;
		if (parser.write_file(*current_map, current_map_path))
		{
			LOG(LOG_INFO, "[MapEditor] Map saved successfully");
		}
		else
		{
			LOG(LOG_ERROR, "[MapEditor] Failed to save map: " << parser.get_error());
		}
	}
	
	void UIStateMapEditor::save_map_as()
	{
		if (!current_map)
		{
			LOG(LOG_ERROR, "[MapEditor] No map to save");
			return;
		}
		
		// TODO: Implement file dialog or text input for file path
		// For now, let's save to a fixed location
		std::string save_path = "maps/" + current_map->get_name() + ".yaml";
		
		LOG(LOG_INFO, "[MapEditor] Saving map as: " << save_path);
		
		MapFormatParser parser;
		if (parser.write_file(*current_map, save_path))
		{
			current_map_path = save_path;
			LOG(LOG_INFO, "[MapEditor] Map saved successfully");
		}
		else
		{
			LOG(LOG_ERROR, "[MapEditor] Failed to save map: " << parser.get_error());
		}
	}
	
	void UIStateMapEditor::switch_to_play_mode()
	{
		if (!current_map)
		{
			LOG(LOG_WARN, "[MapEditor] Cannot enter play mode without a map");
			return;
		}
		
		editor_mode = MODE_PLAY;
		
		// Set player position to spawn point
		player_position = Point<int16_t>(current_map->get_player_spawn_x(), current_map->get_player_spawn_y());
		
		// Hide UI elements
		for (auto& type_elem : elementorder)
		{
			auto& element = elements[type_elem];
			if (element && element->is_active())
			{
				element->toggle_active();
			}
		}
		
		LOG(LOG_INFO, "[MapEditor] Switched to play mode");
	}
	
	void UIStateMapEditor::switch_to_edit_mode()
	{
		editor_mode = MODE_EDIT;
		
		// Show UI elements
		for (auto& type_elem : elementorder)
		{
			auto& element = elements[type_elem];
			if (element && !element->is_active())
			{
				element->toggle_active();
			}
		}
		
		LOG(LOG_INFO, "[MapEditor] Switched to edit mode");
	}
	
	void UIStateMapEditor::handle_play_mode_input(int32_t action, bool pressed)
	{
		// Handle WASD movement keys
		switch (action)
		{
		case 87: // W key
			movement_state.moving_up = pressed;
			break;
		case 65: // A key
			movement_state.moving_left = pressed;
			break;
		case 83: // S key
			movement_state.moving_down = pressed;
			break;
		case 68: // D key
			movement_state.moving_right = pressed;
			break;
		}
	}
	
	void UIStateMapEditor::update_player_movement()
	{
		if (editor_mode != MODE_PLAY || !current_map)
			return;
		
		// Apply movement based on current key states
		const int16_t movement_speed = 3; // pixels per frame
		Point<int16_t> new_position = player_position;
		
		if (movement_state.moving_left)
			new_position = new_position + Point<int16_t>(-movement_speed, 0);
		if (movement_state.moving_right)
			new_position = new_position + Point<int16_t>(movement_speed, 0);
		if (movement_state.moving_up)
			new_position = new_position + Point<int16_t>(0, -movement_speed);
		if (movement_state.moving_down)
			new_position = new_position + Point<int16_t>(0, movement_speed);
		
		// Basic collision detection with map bounds
		if (new_position.x() >= 0 && new_position.x() < current_map->get_width() * 40 &&
			new_position.y() >= 0 && new_position.y() < current_map->get_height() * 40)
		{
			player_position = new_position;
		}
	}
	
	void UIStateMapEditor::draw_player() const
	{
		if (editor_mode != MODE_PLAY)
			return;
		
		// Draw a simple player representation (colored rectangle)
		Point<int16_t> screen_pos = Point<int16_t>(100, 100) + player_position; // Map offset + player pos
		
		// Draw player as a blue square
		GraphicsGL::get().drawrectangle(
			screen_pos.x() - 8,
			screen_pos.y() - 16,
			16,  // width
			32,  // height
			0.0f, 0.0f, 1.0f, 1.0f  // blue color
		);
	}
	
	void UIStateMapEditor::add_asset_to_palette(int32_t asset_id)
	{
		if (!current_map)
		{
			LOG(LOG_WARN, "[MapEditor] Cannot add asset to palette: no map loaded");
			return;
		}
		
		// Find a free symbol for the palette entry
		char symbol = 'A';
		auto& palette = current_map->get_palette();
		
		// Check if asset is already in palette
		for (const auto& entry : palette)
		{
			if (entry.second.id == asset_id)
			{
				LOG(LOG_INFO, "[MapEditor] Asset " << asset_id << " already in palette as '" << entry.first << "'");
				return;
			}
		}
		
		// Find next available symbol
		while (symbol <= 'Z' && palette.find(symbol) != palette.end())
		{
			symbol++;
		}
		
		if (symbol > 'Z')
		{
			// Try lowercase letters
			symbol = 'a';
			while (symbol <= 'z' && palette.find(symbol) != palette.end())
			{
				symbol++;
			}
			
			if (symbol > 'z')
			{
				LOG(LOG_WARN, "[MapEditor] Palette is full, cannot add more assets");
				return;
			}
		}
		
		// Create palette entry with a descriptive name
		std::string asset_name = "Asset_" + std::to_string(asset_id);
		PaletteEntry entry(asset_id, asset_name);
		
		// Add to map palette
		current_map->add_palette_entry(symbol, entry);
		
		// Update tile palette UI
		if (auto* palette_ui = dynamic_cast<UITilePalette*>(get(UIElement::Type::MAP_EDITOR_PALETTE)))
		{
			palette_ui->set_palette(&current_map->get_palette());
		}
		
		LOG(LOG_INFO, "[MapEditor] Added asset " << asset_id << " to palette as '" << symbol << "'");
	}
}