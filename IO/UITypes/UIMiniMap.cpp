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
#include "UIMiniMap.h"

#include "UIWorldMap.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/MapleMap/Npc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif


namespace ms
{
	UIMiniMap::UIMiniMap(const CharStats& stats) : UIDragElement<PosMINIMAP>(Point<int16_t>(128, 20)), stats(stats)
	{
		big_map = true;
		has_map = false;
		listNpc_enabled = false;
		listNpc_dimensions = Point<int16_t>(150, 170);
		listNpc_offset = 0;
		selected = -1;
		
		// Initialize border widths with defaults
		left_border_width = 64;
		right_border_width = 64;
		total_border_width = 128;

		type = Setting<MiniMapType>::get().load();
		user_type = type;
		simpleMode = Setting<MiniMapSimpleMode>::get().load();

		// Always use UIWindow2.img for MiniMap - this is the correct location
		nl::node UIWindow = nl::nx::UI["UIWindow2.img"];
		
		std::string node = simpleMode ? "MiniMapSimpleMode" : "MiniMap";
		MiniMap = UIWindow[node];
		listNpc = UIWindow["MiniMap"]["ListNpc"];
		
		// V87 compatibility: If UIWindow2.img doesn't exist, try UIWindow.img as fallback
		if (MiniMap.name().empty()) {
			UIWindow = nl::nx::UI["UIWindow.img"];
			if (!UIWindow.name().empty()) {
				MiniMap = UIWindow[node];
				listNpc = UIWindow["MiniMap"]["ListNpc"];
			}
		}
		
		MapHelper = nl::nx::Map["MapHelper.img"];

		// Only create buttons if the nodes exist
		if (!MiniMap.name().empty()) {
			buttons[Buttons::BT_MIN] = std::make_unique<MapleButton>(MiniMap["BtMin"], Point<int16_t>(195, -6));
			buttons[Buttons::BT_MAX] = std::make_unique<MapleButton>(MiniMap["BtMax"], Point<int16_t>(209, -6));
			buttons[Buttons::BT_SMALL] = std::make_unique<MapleButton>(MiniMap["BtSmall"], Point<int16_t>(223, -6));
			buttons[Buttons::BT_BIG] = std::make_unique<MapleButton>(MiniMap["BtBig"], Point<int16_t>(223, -6));
			buttons[Buttons::BT_MAP] = std::make_unique<MapleButton>(MiniMap["BtMap"], Point<int16_t>(237, -6));
			buttons[Buttons::BT_NPC] = std::make_unique<MapleButton>(MiniMap["BtNpc"], Point<int16_t>(276, -6));

			// +/- buttons using SoftKeyboard textures (same as chat box)
			nl::node softkey_btns = nl::nx::UI["UIWindow.img"]["SoftKeyboard"]["Bt"]["0"];
			if (softkey_btns["BtMin"] && softkey_btns["BtMax"]) {
				buttons[Buttons::BT_MINUS] = std::make_unique<MapleButton>(softkey_btns["BtMin"], Point<int16_t>(195, -6));
				buttons[Buttons::BT_PLUS] = std::make_unique<MapleButton>(softkey_btns["BtMax"], Point<int16_t>(209, -6));
			}
		}

		region_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK);
		town_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK);
		combined_text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK);
		title_text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::BLACK, "Minimap");

		// Use the correct UIWindow reference for marker
		nl::node markerWindow = nl::nx::UI["UIWindow2.img"];
		if (markerWindow.name().empty()) {
			markerWindow = nl::nx::UI["UIWindow.img"];
		}
		marker = Setting<MiniMapDefaultHelpers>::get().load() ? markerWindow["MiniMapSimpleMode"]["DefaultHelper"] : MapHelper["minimap"];

		player_marker = Animation(marker["user"]);
		selected_marker = Animation(MiniMap["iconNpc"]);
	}

	void UIMiniMap::draw(float alpha) const
	{
		if (type == Type::MIN)
		{
			for (Sprite sprite : min_sprites)
				sprite.draw(position, alpha);

			combined_text.draw(position + Point<int16_t>(7, -3));
		}
		else if (type == Type::NORMAL)
		{
			// Draw border sprites (but not canvas when using viewport)
			for (size_t i = 0; i < normal_sprites.size(); i++)
			{
				// Skip canvas sprite (index 1) when using viewport - we'll draw it manually with clipping
				if (use_viewport && has_map && i == 1)
					continue;
				normal_sprites[i].draw(position, alpha);
			}

			// Draw map name in title area
			combined_text.draw(position + Point<int16_t>(7, -3));

			if (has_map)
			{
				// Calculate viewport offset to center on player
				if (use_viewport)
				{
					Point<int16_t> player_pos = Stage::get().get_player().get_position();
					Point<int16_t> player_map_pos = (player_pos + center_offset) / scale;

					// Calculate offset to center the player in the viewport
					viewport_offset = Point<int16_t>(
						std::max<int16_t>(0, std::min<int16_t>(full_map_size.x() - viewport_size.x(),
							player_map_pos.x() - viewport_size.x() / 2)),
						std::max<int16_t>(0, std::min<int16_t>(full_map_size.y() - viewport_size.y(),
							player_map_pos.y() - viewport_size.y() / 2))
					);

					// Use Range parameters to crop the canvas texture
					// horizontal: first() = crop from left, second() = crop from right
					// vertical: first() = crop from top, second() = crop from bottom
					// Add 2px padding on the right side
					Range<int16_t> h_range(viewport_offset.x(),
						full_map_size.x() - viewport_offset.x() - viewport_size.x() + 2);
					Range<int16_t> v_range(viewport_offset.y(),
						full_map_size.y() - viewport_offset.y() - viewport_size.y());

					// Range parameters shift the draw position, so compensate by subtracting viewport_offset
					// This way: final_pos = (draw_pos - viewport_offset) + viewport_offset = draw_pos
					Point<int16_t> canvas_draw_pos = position + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) - viewport_offset;
					map_sprite.draw(DrawArgument(canvas_draw_pos), v_range, h_range);

					// Base position for markers within the viewport window
					Point<int16_t> marker_base = position + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);

					// Draw portal markers - only if within viewport bounds (minus 2px right padding)
					Animation portal_marker = Animation(marker["portal"]);
					for (auto& sprite : static_marker_info)
					{
						// sprite.second contains the marker position relative to map_draw_origin
						Point<int16_t> marker_map_pos = sprite.second - Point<int16_t>(map_draw_origin_x, map_draw_origin_y);

						// Check if marker is within viewport bounds (with 2px right padding)
						if (marker_map_pos.x() >= viewport_offset.x() &&
							marker_map_pos.x() < viewport_offset.x() + viewport_size.x() - 2 &&
							marker_map_pos.y() >= viewport_offset.y() &&
							marker_map_pos.y() < viewport_offset.y() + viewport_size.y())
						{
							// Draw at screen position: base + (marker_pos - viewport_offset)
							Point<int16_t> screen_pos = marker_base + marker_map_pos - viewport_offset;
							portal_marker.draw(screen_pos, alpha);
						}
					}

					// Draw movable markers with viewport info
					draw_movable_markers(position, alpha);
				}
				else
				{
					Animation portal_marker = Animation(marker["portal"]);

					for (auto& sprite : static_marker_info)
						portal_marker.draw(position + sprite.second, alpha);

					draw_movable_markers(position, alpha);
				}

				if (listNpc_enabled)
					draw_npclist(normal_dimensions, alpha);
			}
		}
		else
		{
			// Draw border sprites (but not canvas when using viewport)
			for (size_t i = 0; i < max_sprites.size(); i++)
			{
				// Skip canvas sprite (index 1) when using viewport - we'll draw it manually with clipping
				if (use_viewport && has_map && i == 1)
					continue;
				max_sprites[i].draw(position, alpha);
			}

			region_text.draw(position + Point<int16_t>(48, 14));
			town_text.draw(position + Point<int16_t>(48, 28));

			if (has_map)
			{
				// Calculate viewport offset to center on player
				if (use_viewport)
				{
					Point<int16_t> player_pos = Stage::get().get_player().get_position();
					Point<int16_t> player_map_pos = (player_pos + center_offset) / scale;

					// Calculate offset to center the player in the viewport
					viewport_offset = Point<int16_t>(
						std::max<int16_t>(0, std::min<int16_t>(full_map_size.x() - viewport_size.x(),
							player_map_pos.x() - viewport_size.x() / 2)),
						std::max<int16_t>(0, std::min<int16_t>(full_map_size.y() - viewport_size.y(),
							player_map_pos.y() - viewport_size.y() / 2))
					);

					// Use Range parameters to crop the canvas texture
					// Add 2px padding on the right side
					Range<int16_t> h_range(viewport_offset.x(),
						full_map_size.x() - viewport_offset.x() - viewport_size.x() + 2);
					Range<int16_t> v_range(viewport_offset.y(),
						full_map_size.y() - viewport_offset.y() - viewport_size.y());

					// Range parameters shift the draw position, so compensate by subtracting viewport_offset
					Point<int16_t> canvas_draw_pos = position + Point<int16_t>(map_draw_origin_x, map_draw_origin_y + MAX_ADJ) - viewport_offset;
					map_sprite.draw(DrawArgument(canvas_draw_pos), v_range, h_range);

					// Base position for markers within the viewport window
					Point<int16_t> marker_base = position + Point<int16_t>(map_draw_origin_x, map_draw_origin_y + MAX_ADJ);

					// Draw portal markers - only if within viewport bounds (minus 2px right padding)
					Animation portal_marker(marker["portal"]);
					for (auto& sprite : static_marker_info)
					{
						Point<int16_t> marker_map_pos = sprite.second - Point<int16_t>(map_draw_origin_x, map_draw_origin_y);

						// Check if marker is within viewport bounds (with 2px right padding)
						if (marker_map_pos.x() >= viewport_offset.x() &&
							marker_map_pos.x() < viewport_offset.x() + viewport_size.x() - 2 &&
							marker_map_pos.y() >= viewport_offset.y() &&
							marker_map_pos.y() < viewport_offset.y() + viewport_size.y())
						{
							// Draw at screen position: base + (marker_pos - viewport_offset)
							Point<int16_t> screen_pos = marker_base + marker_map_pos - viewport_offset;
							portal_marker.draw(screen_pos, alpha);
						}
					}

					// Draw movable markers with viewport info
					draw_movable_markers(position + Point<int16_t>(0, MAX_ADJ), alpha);
				}
				else
				{
					Animation portal_marker(marker["portal"]);

					for (auto& sprite : static_marker_info)
						portal_marker.draw(position + sprite.second + Point<int16_t>(0, MAX_ADJ), alpha);

					draw_movable_markers(position + Point<int16_t>(0, MAX_ADJ), alpha);
				}

				if (listNpc_enabled)
					draw_npclist(max_dimensions, alpha);
			}
		}

		UIElement::draw(alpha);
	}

	void UIMiniMap::update()
	{
		int32_t mid = Stage::get().get_mapid();

		if (mid != mapid)
		{
			mapid = mid;
			Map = NxHelper::Map::get_map_node_name(mapid);

			nl::node town = Map["info"]["town"];
			nl::node miniMap = Map["miniMap"];

			if (!miniMap)
			{
				has_map = false;
				type = Type::MIN;
			}
			else
			{
				has_map = true;

				if (town && town.get_bool())
					type = Type::MAX;
				else
					type = user_type;
			}

			scale = std::pow(2, (int)miniMap["mag"]);
			center_offset = Point<int16_t>(miniMap["centerX"], miniMap["centerY"]);

			update_text();
			update_buttons();
			update_canvas();
			update_static_markers();
			toggle_buttons();
			update_npclist();
		}

		if (type == Type::MIN)
		{
			for (Sprite sprite : min_sprites)
				sprite.update();
		}
		else if (type == Type::NORMAL)
		{
			for (Sprite sprite : normal_sprites)
				sprite.update();
		}
		else
		{
			for (Sprite sprite : max_sprites)
				sprite.update();
		}

		if (listNpc_enabled)
			for (Sprite sprite : listNpc_sprites)
				sprite.update();

		if (selected >= 0)
			selected_marker.update();

		UIElement::update();
	}

	void UIMiniMap::remove_cursor()
	{
		UIDragElement::remove_cursor();

		listNpc_slider.remove_cursor();

		UI::get().clear_tooltip(Tooltip::Parent::MINIMAP);
	}

	Cursor::State UIMiniMap::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		Point<int16_t> cursor_relative = cursorpos - position;

		if (listNpc_slider.isenabled())
			if (Cursor::State new_state = listNpc_slider.send_cursor(cursor_relative, clicked))
				return new_state;

		if (listNpc_enabled)
		{
			Point<int16_t> relative_point = cursor_relative - Point<int16_t>(10 + (type == Type::MAX ? max_dimensions : normal_dimensions).x(), 23);
			Rectangle<int16_t> list_bounds = Rectangle<int16_t>(0, LISTNPC_ITEM_WIDTH, 0, LISTNPC_ITEM_HEIGHT * 8);

			if (list_bounds.contains(relative_point))
			{
				int16_t list_index = listNpc_offset + relative_point.y() / LISTNPC_ITEM_HEIGHT;
				bool in_list = list_index < listNpc_names.size();

				if (clicked)
					select_npclist(in_list ? list_index : -1);
				else if (in_list)
					UI::get().show_text(Tooltip::Parent::MINIMAP, listNpc_full_names[list_index]);

				return Cursor::State::IDLE;
			}
		}

		bool found = false;
		auto npcs = Stage::get().get_npcs().get_npcs();

		for (auto npc = npcs->begin(); npc != npcs->end(); npc++)
		{
			Point<int16_t> npc_pos = (npc->second->get_position() + center_offset) / scale + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
			Rectangle<int16_t> marker_spot = Rectangle<int16_t>(npc_pos - Point<int16_t>(4, 8), npc_pos);

			if (type == Type::MAX)
				marker_spot.shift(Point<int16_t>(0, MAX_ADJ));

			if (marker_spot.contains(cursor_relative))
			{
				found = true;

				auto n = static_cast<Npc*>(npc->second.get());
				std::string name = n->get_name();
				std::string func = n->get_func();

				UI::get().show_map(Tooltip::Parent::MINIMAP, name, func, {}, false, false);
				break;
			}
		}

		if (!found)
		{
			for (auto& sprite : static_marker_info)
			{
				Rectangle<int16_t> marker_spot = Rectangle<int16_t>(sprite.second, sprite.second + 8);

				if (type == Type::MAX)
					marker_spot.shift(Point<int16_t>(0, MAX_ADJ));

				if (marker_spot.contains(cursor_relative))
				{
					nl::node portal_tm = Map["portal"][sprite.first]["tm"];
					std::string portal_cat = NxHelper::Map::get_map_category(portal_tm);
					nl::node portal_name = nl::nx::String["Map.img"][portal_cat][portal_tm]["mapName"];

					if (portal_name)
					{
						found = true;

						UI::get().show_map(Tooltip::Parent::MINIMAP, portal_name, "", portal_tm, false, true);
						break;
					}
				}
			}
		}

		return Cursor::State::IDLE;
	}

	void UIMiniMap::send_scroll(double yoffset)
	{
		if (listNpc_enabled && listNpc_slider.isenabled())
			listNpc_slider.send_scroll(yoffset);
	}

	void UIMiniMap::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (has_map)
		{
			if (type < Type::MAX)
				type++;
			else
				type = Type::MIN;

			user_type = type;

			toggle_buttons();
		}
	}

	Button::State UIMiniMap::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case BT_MIN:
			{
				type -= 1;
				toggle_buttons();

				return type == Type::MIN ? Button::State::DISABLED : Button::State::NORMAL;
			}
			case BT_MAX:
			{
				type += 1;
				toggle_buttons();

				return type == Type::MAX ? Button::State::DISABLED : Button::State::NORMAL;
			}
			case BT_SMALL:
			case BT_BIG:
			{
				big_map = !big_map;
				// TODO: Toggle scrolling map
				toggle_buttons();
				break;
			}
			case BT_MAP:
			{
				UI::get().emplace<UIWorldMap>();
				break;
			}
			case BT_NPC:
			{
				set_npclist_active(!listNpc_enabled);
				break;
			}
			case BT_MINUS:
			{
				// Collapse minimap mode (same as BT_MIN)
				if (type > Type::MIN)
				{
					type -= 1;
					toggle_buttons();
				}
				return type == Type::MIN ? Button::State::DISABLED : Button::State::NORMAL;
			}
			case BT_PLUS:
			{
				// Expand minimap mode (same as BT_MAX)
				if (type < Type::MAX)
				{
					type += 1;
					toggle_buttons();
				}
				return type == Type::MAX ? Button::State::DISABLED : Button::State::NORMAL;
			}
		}

		return Button::State::NORMAL;
	}

	UIElement::Type UIMiniMap::get_type() const
	{
		return TYPE;
	}

	void UIMiniMap::update_buttons()
	{
		// Add one pixel for a space to the right of each button
		auto btn_min = buttons.find(Buttons::BT_MIN);
		auto btn_max = buttons.find(Buttons::BT_MAX);
		auto btn_map = buttons.find(Buttons::BT_MAP);
		auto btn_plus = buttons.find(Buttons::BT_PLUS);
		auto btn_minus = buttons.find(Buttons::BT_MINUS);

		bt_min_width = (btn_min != buttons.end() && btn_min->second) ? btn_min->second->width() + 1 : 0;
		bt_max_width = (btn_max != buttons.end() && btn_max->second) ? btn_max->second->width() + 1 : 0;
		bt_map_width = (btn_map != buttons.end() && btn_map->second) ? btn_map->second->width() + 1 : 0;
		bt_plus_width = (btn_plus != buttons.end() && btn_plus->second) ? btn_plus->second->width() + 1 : 15;  // fallback 15px
		bt_minus_width = (btn_minus != buttons.end() && btn_minus->second) ? btn_minus->second->width() + 1 : 15;  // fallback 15px

		combined_text_width = combined_text.width();
	}

	void UIMiniMap::toggle_buttons()
	{
		// Helper lambda for safe button access
		auto safe_btn = [this](uint16_t id) -> Button* {
			auto it = buttons.find(id);
			return (it != buttons.end() && it->second) ? it->second.get() : nullptr;
		};

		if (type == Type::MIN)
		{
			if (auto btn = safe_btn(Buttons::BT_MAP)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_MAX)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_NPC)) btn->set_active(false);
			if (auto btn = safe_btn(Buttons::BT_SMALL)) btn->set_active(false);
			if (auto btn = safe_btn(Buttons::BT_BIG)) btn->set_active(false);
			// +/- buttons active next to world button
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_PLUS)) btn->set_active(true);

			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_state(Button::State::DISABLED);
			// Minus disabled at MIN mode
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_state(Button::State::DISABLED);

			if (auto btn = safe_btn(Buttons::BT_MAX)) {
				if (has_map)
					btn->set_state(Button::State::NORMAL);
				else
					btn->set_state(Button::State::DISABLED);
			}
			// Plus enabled (can expand from MIN)
			if (auto btn = safe_btn(Buttons::BT_PLUS)) {
				if (has_map)
					btn->set_state(Button::State::NORMAL);
				else
					btn->set_state(Button::State::DISABLED);
			}

			// Position buttons from right edge: MAP, -, +, MIN, MAX
			int16_t total_btn_width = bt_map_width + bt_minus_width + bt_plus_width + bt_min_width + bt_max_width;
			int16_t min_width = combined_text_width + 11 + total_btn_width + 7 + 100;  // Add 100px extra width
			int16_t btn_x = min_width - 7;

			// MAP button (world) - rightmost
			btn_x -= bt_map_width;
			if (auto btn = safe_btn(Buttons::BT_MAP)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// MINUS button (-) - to the left of MAP
			btn_x -= bt_minus_width;
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// PLUS button (+) - to the left of MINUS
			btn_x -= bt_plus_width;
			if (auto btn = safe_btn(Buttons::BT_PLUS)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// MAX button (expand window) - next
			btn_x -= bt_max_width;
			if (auto btn = safe_btn(Buttons::BT_MAX)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// MIN button (minimize window) - leftmost
			btn_x -= bt_min_width;
			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			min_dimensions = Point<int16_t>(min_width, 20);

			update_dimensions();

			dragarea = dimension;

			set_npclist_active(false);
		}
		else
		{
			bool has_npcs = Stage::get().get_npcs().get_npcs()->size() > 0;

			if (auto btn = safe_btn(Buttons::BT_MAP)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_MAX)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_NPC)) btn->set_active(has_npcs);
			// +/- buttons active next to world button
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_active(true);
			if (auto btn = safe_btn(Buttons::BT_PLUS)) btn->set_active(true);

			if (big_map)
			{
				if (auto btn = safe_btn(Buttons::BT_BIG)) btn->set_active(false);
				if (auto btn = safe_btn(Buttons::BT_SMALL)) btn->set_active(true);
			}
			else
			{
				if (auto btn = safe_btn(Buttons::BT_BIG)) btn->set_active(true);
				if (auto btn = safe_btn(Buttons::BT_SMALL)) btn->set_active(false);
			}

			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_state(Button::State::NORMAL);
			// Minus always enabled in NORMAL/MAX mode (can always collapse)
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_state(Button::State::NORMAL);

			// Get button widths
			auto btn_small = safe_btn(Buttons::BT_SMALL);
			auto btn_npc = safe_btn(Buttons::BT_NPC);
			int16_t small_width = btn_small ? btn_small->width() + 1 : 0;
			int16_t npc_width = (has_npcs && btn_npc) ? btn_npc->width() + 1 : 0;

			// Position buttons from right edge inside title area
			// Order: NPC (if any), SMALL/BIG, MIN, MAX, +, -, MAP
			int16_t window_width;
			if (type == Type::MAX && max_dimensions.x() > 0)
				window_width = max_dimensions.x();
			else if (type == Type::NORMAL && normal_dimensions.x() > 0)
				window_width = normal_dimensions.x();
			else
				window_width = middle_right_x + 55;
			int16_t btn_x = window_width - 5;  // Start from right edge with 5px margin

			// MAP button (world) - rightmost, offset by (1, 2)
			btn_x -= bt_map_width;
			if (auto btn = safe_btn(Buttons::BT_MAP)) btn->set_position(Point<int16_t>(btn_x + 1, BTN_MIN_Y + 2));

			// MINUS button (-) - to the left of MAP
			btn_x -= bt_minus_width;
			if (auto btn = safe_btn(Buttons::BT_MINUS)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// PLUS button (+) - to the left of MINUS
			btn_x -= bt_plus_width;
			if (auto btn = safe_btn(Buttons::BT_PLUS)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// MAX button (expand window) - next
			btn_x -= bt_max_width;
			if (auto btn = safe_btn(Buttons::BT_MAX)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// MIN button (minimize window) - next
			btn_x -= bt_min_width;
			if (auto btn = safe_btn(Buttons::BT_MIN)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// SMALL/BIG button - next
			btn_x -= small_width;
			if (auto btn = safe_btn(Buttons::BT_SMALL)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));
			if (auto btn = safe_btn(Buttons::BT_BIG)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));

			// NPC button - leftmost (if active)
			if (has_npcs)
			{
				btn_x -= npc_width;
				if (auto btn = safe_btn(Buttons::BT_NPC)) btn->set_position(Point<int16_t>(btn_x, BTN_MIN_Y));
			}

			if (auto btn = safe_btn(Buttons::BT_MAX)) {
				if (type == Type::MAX)
					btn->set_state(Button::State::DISABLED);
				else
					btn->set_state(Button::State::NORMAL);
			}
			// Plus disabled at MAX mode
			if (auto btn = safe_btn(Buttons::BT_PLUS)) {
				if (type == Type::MAX)
					btn->set_state(Button::State::DISABLED);
				else
					btn->set_state(Button::State::NORMAL);
			}

			set_npclist_active(listNpc_enabled && has_npcs);

			dragarea = Point<int16_t>(dimension.x(), 20);
		}
	}

	void UIMiniMap::update_text()
	{
		NxHelper::Map::MapInfo map_info = NxHelper::Map::get_map_info_by_id(mapid);
		combined_text.change_text(map_info.full_name);
		region_text.change_text(map_info.name);
		town_text.change_text(map_info.street_name);
	}

	void UIMiniMap::update_canvas()
	{
		min_sprites.clear();
		normal_sprites.clear();
		max_sprites.clear();

		nl::node Min, Normal, Max;

		if (simpleMode)
		{
			Min = MiniMap["Window"]["Min"];
			Normal = MiniMap["Window"]["Normal"];
			Max = MiniMap["Window"]["Max"];
		}
		else
		{
			Min = MiniMap["Min"];
			Normal = MiniMap["MinMap"];
			Max = MiniMap["MaxMap"];
		}

		// Load minimap data from the NX file
		nl::node miniMapNode = Map["miniMap"];
		map_sprite = Texture(miniMapNode["canvas"]);

		// Get canvas dimensions first
		Point<int16_t> map_dimensions = map_sprite.get_dimensions();

		// Store full map size for viewport calculations
		full_map_size = map_dimensions;

		// Check if map is larger than max viewport size
		use_viewport = (map_dimensions.x() > MAX_MINIMAP_WIDTH || map_dimensions.y() > MAX_MINIMAP_HEIGHT);

		// Calculate viewport size (clamped to max dimensions)
		viewport_size = Point<int16_t>(
			std::min(map_dimensions.x(), MAX_MINIMAP_WIDTH),
			std::min(map_dimensions.y(), MAX_MINIMAP_HEIGHT)
		);

		// Initialize viewport offset to center
		viewport_offset = Point<int16_t>(0, 0);

		// Check if width/height are specified in the minimap node
		int16_t minimap_width = miniMapNode["width"];
		int16_t minimap_height = miniMapNode["height"];

		// The width/height in the minimap node might be world coordinates, not pixel dimensions
		// If they're unreasonably large or not specified, use canvas dimensions
		// When using viewport, limit the displayed size
		if (use_viewport) {
			minimap_width = viewport_size.x();
			minimap_height = viewport_size.y();
		} else {
			if (minimap_width <= 0 || minimap_width > 500) {
				minimap_width = map_dimensions.x();
			}
			if (minimap_height <= 0 || minimap_height > 500) {
				minimap_height = map_dimensions.y();
			}
		}
		
		// Get actual border sprite dimensions instead of using hardcoded values
		Texture leftBorder(Normal[simpleMode ? "UpLeft" : "nw"]);
		Texture rightBorder(Normal[simpleMode ? "UpRight" : "ne"]);
		
		// Update member variables with actual border dimensions
		left_border_width = leftBorder.width();
		right_border_width = rightBorder.width();
		total_border_width = left_border_width + right_border_width;
		
		// If we couldn't get border dimensions, fall back to defaults
		if (total_border_width <= 0) {
			left_border_width = 64;
			right_border_width = 64;
			total_border_width = 128;
		}

		// 48 (offset for text) + longer text's width + 10 (space for right side border)
		int16_t mark_text_width = 48 + std::max<int16_t>(region_text.width(), town_text.width()) + 10;
		int16_t c_stretch, ur_x_offset, m_stretch, down_y_offset;
		
		// Calculate window width to fit the minimap canvas plus borders
		int16_t content_width = std::max<int16_t>(mark_text_width, minimap_width);
		int16_t window_width = std::max<int16_t>(178, content_width + total_border_width);

		c_stretch = std::max<int16_t>(0, window_width - total_border_width);
		ur_x_offset = left_border_width + c_stretch;
		map_draw_origin_x = std::max<int16_t>(10, left_border_width + (c_stretch / 2) - (minimap_width / 2));

		// Get actual top and bottom border heights
		Texture topBorder(Normal[simpleMode ? "UpCenter" : "n"]);
		Texture bottomBorder(Normal[simpleMode ? "DownCenter" : "s"]);
		int16_t top_border_height = topBorder.height();
		int16_t bottom_border_height = bottomBorder.height();
		if (top_border_height <= 0) top_border_height = 25; // fallback
		if (bottom_border_height <= 0) bottom_border_height = 27; // fallback
		
		// Calculate the middle section stretch to accommodate the minimap
		// We need the canvas to fit within the border area
		// The content area starts at ML_MR_Y and we need space for the actual canvas
		// Use minimap_height (which is viewport_size when use_viewport is true)
		int16_t padding = 10; // Padding above and below the canvas
		int16_t required_middle_height = minimap_height + padding - (ML_MR_Y - top_border_height);
		m_stretch = std::max<int16_t>(required_middle_height, 5);
		
		// Calculate where the bottom border should be positioned
		// It should connect exactly where the middle borders end
		down_y_offset = ML_MR_Y + m_stretch;
		
		// Position minimap canvas within the window borders
		// Place it so it doesn't overflow the bottom
		// Use minimap_height (which is viewport_size when use_viewport is true)
		map_draw_origin_y = down_y_offset - minimap_height - 5; // 5 pixels padding from bottom

		// Get the actual middle right border width
		Texture middleRightBorder(Normal[simpleMode ? "MiddleRight" : "e"]);
		int16_t middle_right_width = middleRightBorder.width();
		if (middle_right_width <= 0) middle_right_width = 7; // fallback
		
		middle_right_x = ur_x_offset + right_border_width - middle_right_width;

		std::string Left = simpleMode ? "Left" : "w";
		std::string Center = simpleMode ? "Center" : "c";
		std::string Right = simpleMode ? "Right" : "e";

		std::string DownCenter = simpleMode ? "DownCenter" : "s";
		std::string DownLeft = simpleMode ? "DownLeft" : "sw";
		std::string DownRight = simpleMode ? "DownRight" : "se";
		std::string MiddleLeft = simpleMode ? "MiddleLeft" : "w";
		std::string MiddleRight = simpleMode ? "MiddleRight" : "e";
		std::string UpCenter = simpleMode ? "UpCenter" : "n";
		std::string UpLeft = simpleMode ? "UpLeft" : "nw";
		std::string UpRight = simpleMode ? "UpRight" : "ne";

		// SimpleMode's backdrop is opaque, the other is transparent but lightly colored
		// UI.wz v208 has normal center sprite in-linked to bottom right window frame, not sure why.
		nl::node MiddleCenter = simpleMode ? MiniMap["Window"]["Max"]["MiddleCenter"] : MiniMap["MaxMap"]["c"];

		int16_t dl_dr_y = std::max(minimap_height, (int16_t)10);

		// combined_text_width + 14 (7px buffer on both sides) + 4 (buffer between name and buttons) + all buttons' widths + 100px extra - total border width
		int16_t min_c_stretch = combined_text_width + 18 + bt_min_width + bt_max_width + bt_map_width + bt_plus_width + bt_minus_width + 100 - total_border_width;

		// Min sprites queue
		min_sprites.emplace_back(Min[Center], DrawArgument(WINDOW_UL_POS + Point<int16_t>(left_border_width, 0), Point<int16_t>(min_c_stretch, 0)));
		min_sprites.emplace_back(Min[Left], DrawArgument(WINDOW_UL_POS));
		min_sprites.emplace_back(Min[Right], DrawArgument(WINDOW_UL_POS + Point<int16_t>(min_c_stretch + left_border_width, 0)));

		// Normal sprites queue
		// (7, 10) is the top left corner of the inner window
		// The middle content width is c_stretch + total_border_width - 14 (width of middle borders * 2)
		// 27 = height of inner frame drawn on up and down borders
		normal_sprites.emplace_back(MiddleCenter, DrawArgument(Point<int16_t>(7, 10), Point<int16_t>(c_stretch + total_border_width - 14, m_stretch + 27)));

		if (has_map)
			normal_sprites.emplace_back(Map["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y)));

		normal_sprites.emplace_back(Normal[MiddleLeft], DrawArgument(Point<int16_t>(0, ML_MR_Y), Point<int16_t>(0, m_stretch)));
		normal_sprites.emplace_back(Normal[MiddleRight], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y), Point<int16_t>(0, m_stretch)));
		normal_sprites.emplace_back(Normal[UpCenter], DrawArgument(Point<int16_t>(left_border_width, 0) + WINDOW_UL_POS, Point<int16_t>(c_stretch, 0)));
		normal_sprites.emplace_back(Normal[UpLeft], WINDOW_UL_POS);
		normal_sprites.emplace_back(Normal[UpRight], DrawArgument(Point<int16_t>(ur_x_offset, 0) + WINDOW_UL_POS));
		normal_sprites.emplace_back(Normal[DownCenter], DrawArgument(Point<int16_t>(left_border_width, down_y_offset), Point<int16_t>(c_stretch, 0)));
		normal_sprites.emplace_back(Normal[DownLeft], Point<int16_t>(0, down_y_offset));
		normal_sprites.emplace_back(Normal[DownRight], Point<int16_t>(ur_x_offset, down_y_offset));

		normal_dimensions = Point<int16_t>(ur_x_offset + right_border_width, down_y_offset + bottom_border_height);

		// Max sprites queue
		max_sprites.emplace_back(MiddleCenter, DrawArgument(Point<int16_t>(7, 50), Point<int16_t>(c_stretch + total_border_width - 14, m_stretch + 27)));

		if (has_map)
			max_sprites.emplace_back(Map["miniMap"]["canvas"], DrawArgument(Point<int16_t>(map_draw_origin_x, map_draw_origin_y + MAX_ADJ)));

		max_sprites.emplace_back(Max[MiddleLeft], DrawArgument(Point<int16_t>(0, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
		max_sprites.emplace_back(Max[MiddleRight], DrawArgument(Point<int16_t>(middle_right_x, ML_MR_Y + MAX_ADJ), Point<int16_t>(0, m_stretch)));
		max_sprites.emplace_back(Max[UpCenter], DrawArgument(Point<int16_t>(left_border_width, 0) + WINDOW_UL_POS, Point<int16_t>(c_stretch, 0)));
		max_sprites.emplace_back(Max[UpLeft], WINDOW_UL_POS);
		max_sprites.emplace_back(Max[UpRight], DrawArgument(Point<int16_t>(ur_x_offset, 0) + WINDOW_UL_POS));
		max_sprites.emplace_back(Max[DownCenter], DrawArgument(Point<int16_t>(left_border_width, down_y_offset + MAX_ADJ), Point<int16_t>(c_stretch, 0)));
		max_sprites.emplace_back(Max[DownLeft], Point<int16_t>(0, down_y_offset + MAX_ADJ));
		max_sprites.emplace_back(Max[DownRight], Point<int16_t>(ur_x_offset, down_y_offset + MAX_ADJ));
		max_sprites.emplace_back(MapHelper["mark"][Map["info"]["mapMark"]], DrawArgument(Point<int16_t>(7, 17)));

		max_dimensions = normal_dimensions + Point<int16_t>(0, MAX_ADJ);
	}

	void UIMiniMap::draw_movable_markers(Point<int16_t> init_pos, float alpha) const
	{
		if (!has_map)
			return;

		Animation marker_sprite;
		Point<int16_t> sprite_offset;

		// Helper lambda to check if a marker is within viewport bounds (with 2px right padding)
		auto is_in_viewport = [this](Point<int16_t> map_pos) -> bool {
			if (!use_viewport)
				return true;
			return map_pos.x() >= viewport_offset.x() &&
				   map_pos.x() < viewport_offset.x() + viewport_size.x() - 2 &&
				   map_pos.y() >= viewport_offset.y() &&
				   map_pos.y() < viewport_offset.y() + viewport_size.y();
		};

		// Helper lambda to calculate screen position for a marker
		auto get_screen_pos = [this, &init_pos](Point<int16_t> world_pos, Point<int16_t> sprite_off) -> Point<int16_t> {
			Point<int16_t> map_pos = (world_pos + center_offset) / scale;
			if (use_viewport)
			{
				// Position relative to viewport: map_pos - viewport_offset
				return init_pos + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + map_pos - viewport_offset - sprite_off;
			}
			return map_pos - sprite_off + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos;
		};

		/// NPCs
		MapObjects* npcs = Stage::get().get_npcs().get_npcs();
		marker_sprite = Animation(marker["npc"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
		{
			Point<int16_t> npc_pos = npc->second.get()->get_position();
			Point<int16_t> map_pos = (npc_pos + center_offset) / scale;
			if (is_in_viewport(map_pos))
			{
				marker_sprite.draw(get_screen_pos(npc_pos, sprite_offset), alpha);
			}
		}

		/// Other characters
		MapObjects* chars = Stage::get().get_chars().get_chars();
		marker_sprite = Animation(marker["another"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto chr = chars->begin(); chr != chars->end(); ++chr)
		{
			Point<int16_t> chr_pos = chr->second.get()->get_position();
			Point<int16_t> map_pos = (chr_pos + center_offset) / scale;
			if (is_in_viewport(map_pos))
			{
				marker_sprite.draw(get_screen_pos(chr_pos, sprite_offset), alpha);
			}
		}

		/// Player
		Point<int16_t> player_pos = Stage::get().get_player().get_position();
		sprite_offset = player_marker.get_dimensions() / Point<int16_t>(2, 0);
		Point<int16_t> player_map_pos = (player_pos + center_offset) / scale;
		if (is_in_viewport(player_map_pos))
		{
			player_marker.draw(get_screen_pos(player_pos, sprite_offset), alpha);
		}
	}

	void UIMiniMap::update_static_markers()
	{
		static_marker_info.clear();

		if (!has_map)
			return;

		Animation marker_sprite;

		/// Portals
		nl::node portals = Map["portal"];
		marker_sprite = Animation(marker["portal"]);
		Point<int16_t> marker_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (nl::node portal = portals.begin(); portal != portals.end(); ++portal)
		{
			int portal_type = portal["pt"];

			if (portal_type == 2)
			{
				Point<int16_t> marker_pos = (Point<int16_t>(portal["x"], portal["y"]) + center_offset) / scale - marker_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y);
				static_marker_info.emplace_back(portal.name(), marker_pos);
			}
		}
	}

	void UIMiniMap::set_npclist_active(bool active)
	{
		listNpc_enabled = active;

		if (!active)
			select_npclist(-1);

		update_dimensions();
	}

	void UIMiniMap::update_dimensions()
	{
		if (type == Type::MIN)
		{
			dimension = min_dimensions;
		}
		else
		{
			Point<int16_t> base_dims = type == Type::MAX ? max_dimensions : normal_dimensions;
			dimension = base_dims;

			if (listNpc_enabled)
			{
				dimension += listNpc_dimensions;
				dimension.set_y(std::max(base_dims.y(), listNpc_dimensions.y()));
			}
		}
	}

	void UIMiniMap::update_npclist()
	{
		listNpc_sprites.clear();
		listNpc_names.clear();
		listNpc_full_names.clear();
		listNpc_list.clear();
		selected = -1;
		listNpc_offset = 0;

		if (simpleMode)
			return;

		auto npcs = Stage::get().get_npcs().get_npcs();

		for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
		{
			listNpc_list.emplace_back(npc->second.get());

			auto n = static_cast<Npc*>(npc->second.get());
			std::string name = n->get_name();
			std::string func = n->get_func();

			if (func != "")
				name += " (" + func + ")";

			Text name_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, name);

			listNpc_names.emplace_back(name_text);
			listNpc_full_names.emplace_back(name);
		}

		for (size_t i = 0; i < listNpc_names.size(); i++)
			string_format::format_with_ellipsis(listNpc_names[i], LISTNPC_TEXT_WIDTH - (listNpc_names.size() > 8 ? 0 : 20));

		const Point<int16_t> listNpc_pos = Point<int16_t>(type == Type::MAX ? max_dimensions.x() : normal_dimensions.x(), 0);
		int16_t c_stretch = 20;
		int16_t m_stretch = 102;

		if (listNpc_names.size() > 8)
		{
			listNpc_slider = Slider(
				Slider::DEFAULT_SILVER, Range<int16_t>(23, 11 + LISTNPC_ITEM_HEIGHT * 8), listNpc_pos.x() + LISTNPC_ITEM_WIDTH + 1, 8, listNpc_names.size(),
				[&](bool upwards)
				{
					int16_t shift = upwards ? -1 : 1;
					bool above = listNpc_offset + shift >= 0;
					bool below = listNpc_offset + 8 + shift <= listNpc_names.size();

					if (above && below)
						listNpc_offset += shift;
				}
			);

			c_stretch += 12;
		}
		else
		{
			listNpc_slider.setenabled(false);
			m_stretch = LISTNPC_ITEM_HEIGHT * listNpc_names.size() - 34;
			c_stretch -= 17;
		}

		listNpc_sprites.emplace_back(listNpc["c"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, M_START), Point<int16_t>(c_stretch, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["w"], DrawArgument(listNpc_pos + Point<int16_t>(0, M_START), Point<int16_t>(0, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["e"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START), Point<int16_t>(0, m_stretch)));
		listNpc_sprites.emplace_back(listNpc["n"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, 0), Point<int16_t>(c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["s"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X, M_START + m_stretch), Point<int16_t>(c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["nw"], DrawArgument(listNpc_pos + Point<int16_t>(0, 0)));
		listNpc_sprites.emplace_back(listNpc["ne"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, 0)));
		listNpc_sprites.emplace_back(listNpc["sw"], DrawArgument(listNpc_pos + Point<int16_t>(0, M_START + m_stretch)));
		listNpc_sprites.emplace_back(listNpc["se"], DrawArgument(listNpc_pos + Point<int16_t>(CENTER_START_X + c_stretch, M_START + m_stretch)));

		listNpc_dimensions = Point<int16_t>(CENTER_START_X * 2 + c_stretch, M_START + m_stretch + 30);

		update_dimensions();
	}

	void UIMiniMap::draw_npclist(Point<int16_t> minimap_dims, float alpha) const
	{
		Animation npc_marker = Animation(marker["npc"]);

		for (Sprite sprite : listNpc_sprites)
			sprite.draw(position, alpha);

		Point<int16_t> listNpc_pos = position + Point<int16_t>(minimap_dims.x() + 10, 23);

		for (int8_t i = 0; i + listNpc_offset < listNpc_list.size() && i < 8; i++)
		{
			if (selected - listNpc_offset == i)
			{
				ColorBox highlight = ColorBox(LISTNPC_ITEM_WIDTH - (listNpc_slider.isenabled() ? 0 : 30), LISTNPC_ITEM_HEIGHT, Color::Name::YELLOW, 1.0f);
				highlight.draw(listNpc_pos);
			}

			npc_marker.draw(DrawArgument(listNpc_pos + Point<int16_t>(0, 2), false, npc_marker.get_dimensions() / 2), alpha);
			listNpc_names[listNpc_offset + i].draw(DrawArgument(listNpc_pos + Point<int16_t>(14, -2)));

			listNpc_pos.shift_y(LISTNPC_ITEM_HEIGHT);
		}

		if (listNpc_slider.isenabled())
			listNpc_slider.draw(position);

		if (selected >= 0)
		{
			Point<int16_t> npc_pos =
				(listNpc_list[selected]->get_position() + center_offset) / scale +
				Point<int16_t>(map_draw_origin_x, map_draw_origin_y - npc_marker.get_dimensions().y() + (type == Type::MAX ? MAX_ADJ : 0));

			selected_marker.draw(position + npc_pos, 0.5f);
		}
	}

	void UIMiniMap::select_npclist(int16_t choice)
	{
		if (selected == choice)
			return;

		if (selected >= 0 && selected < listNpc_names.size())
			listNpc_names[selected].change_color(Color::Name::WHITE);

		if (choice > listNpc_names.size() || choice < 0)
		{
			selected = -1;
		}
		else
		{
			selected = choice != selected ? choice : -1;

			if (selected >= 0)
				listNpc_names[selected].change_color(Color::Name::BLACK);
		}
	}
}