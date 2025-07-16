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
		}

		region_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		town_text = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::WHITE);
		combined_text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE);

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
			for (Sprite sprite : normal_sprites)
				sprite.draw(position, alpha);

			if (has_map)
			{
				Animation portal_marker = Animation(marker["portal"]);

				for (auto& sprite : static_marker_info)
					portal_marker.draw(position + sprite.second, alpha);

				draw_movable_markers(position, alpha);

				if (listNpc_enabled)
					draw_npclist(normal_dimensions, alpha);
			}
		}
		else
		{
			for (Sprite sprite : max_sprites)
				sprite.draw(position, alpha);

			region_text.draw(position + Point<int16_t>(48, 14));
			town_text.draw(position + Point<int16_t>(48, 28));

			if (has_map)
			{
				Animation portal_marker(marker["portal"]);

				for (auto& sprite : static_marker_info)
					portal_marker.draw(position + sprite.second + Point<int16_t>(0, MAX_ADJ), alpha);

				draw_movable_markers(position + Point<int16_t>(0, MAX_ADJ), alpha);

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
		bt_min_width = buttons[Buttons::BT_MIN]->width() + 1;
		bt_max_width = buttons[Buttons::BT_MAX]->width() + 1;
		bt_map_width = buttons[Buttons::BT_MAP]->width() + 1;

		combined_text_width = combined_text.width();
	}

	void UIMiniMap::toggle_buttons()
	{
		int16_t bt_min_x;

		if (type == Type::MIN)
		{
			buttons[Buttons::BT_MAP]->set_active(true);
			buttons[Buttons::BT_MAX]->set_active(true);
			buttons[Buttons::BT_MIN]->set_active(true);
			buttons[Buttons::BT_NPC]->set_active(false);
			buttons[Buttons::BT_SMALL]->set_active(false);
			buttons[Buttons::BT_BIG]->set_active(false);

			buttons[Buttons::BT_MIN]->set_state(Button::State::DISABLED);

			if (has_map)
				buttons[Buttons::BT_MAX]->set_state(Button::State::NORMAL);
			else
				buttons[Buttons::BT_MAX]->set_state(Button::State::DISABLED);

			bt_min_x = combined_text_width + 11;

			buttons[Buttons::BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_min_width;

			buttons[Buttons::BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			min_dimensions = Point<int16_t>(bt_min_x + bt_map_width + 7, 20);

			update_dimensions();

			dragarea = dimension;

			set_npclist_active(false);
		}
		else
		{
			bool has_npcs = Stage::get().get_npcs().get_npcs()->size() > 0;

			buttons[Buttons::BT_MAP]->set_active(true);
			buttons[Buttons::BT_MAX]->set_active(true);
			buttons[Buttons::BT_MIN]->set_active(true);
			buttons[Buttons::BT_NPC]->set_active(has_npcs);

			if (big_map)
			{
				buttons[Buttons::BT_BIG]->set_active(false);
				buttons[Buttons::BT_SMALL]->set_active(true);
			}
			else
			{
				buttons[Buttons::BT_BIG]->set_active(true);
				buttons[Buttons::BT_SMALL]->set_active(false);
			}

			buttons[Buttons::BT_MIN]->set_state(Button::State::NORMAL);

			// Calculate total button width needed
			int16_t total_button_width = bt_min_width + buttons[Buttons::BT_SMALL]->width() + 1 + bt_max_width + bt_map_width + (has_npcs ? buttons[Buttons::BT_NPC]->width() : 0);
			
			// Position buttons to fit within the window dimensions, leaving some margin
			// Use the window width from normal_dimensions if available, otherwise use middle_right_x
			int16_t window_width = (type == Type::NORMAL && normal_dimensions.x() > 0) ? normal_dimensions.x() : middle_right_x + 55;
			bt_min_x = window_width - total_button_width - 10;

			buttons[Buttons::BT_MIN]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_MAX]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_SMALL]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));
			buttons[Buttons::BT_BIG]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_max_width;

			buttons[Buttons::BT_MAP]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			bt_min_x += bt_map_width;

			buttons[Buttons::BT_NPC]->set_position(Point<int16_t>(bt_min_x, BTN_MIN_Y));

			if (type == Type::MAX)
				buttons[Buttons::BT_MAX]->set_state(Button::State::DISABLED);
			else
				buttons[Buttons::BT_MAX]->set_state(Button::State::NORMAL);

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
		
		// Check if width/height are specified in the minimap node
		int16_t minimap_width = miniMapNode["width"];
		int16_t minimap_height = miniMapNode["height"];
		
		// The width/height in the minimap node might be world coordinates, not pixel dimensions
		// If they're unreasonably large or not specified, use canvas dimensions
		if (minimap_width <= 0 || minimap_width > 500) {
			minimap_width = map_dimensions.x();
		}
		if (minimap_height <= 0 || minimap_height > 500) {
			minimap_height = map_dimensions.y();
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
		int16_t canvas_height = map_dimensions.y(); // Use actual canvas height
		int16_t padding = 10; // Padding above and below the canvas
		int16_t required_middle_height = canvas_height + padding - (ML_MR_Y - top_border_height);
		m_stretch = std::max<int16_t>(required_middle_height, 5);
		
		// Calculate where the bottom border should be positioned
		// It should connect exactly where the middle borders end
		down_y_offset = ML_MR_Y + m_stretch;
		
		// Position minimap canvas within the window borders
		// Place it so it doesn't overflow the bottom
		map_draw_origin_y = down_y_offset - canvas_height - 5; // 5 pixels padding from bottom

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

		// combined_text_width + 14 (7px buffer on both sides) + 4 (buffer between name and buttons) + 3 buttons' widths - total border width
		int16_t min_c_stretch = combined_text_width + 18 + bt_min_width + bt_max_width + bt_map_width - total_border_width;

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

		/// NPCs
		MapObjects* npcs = Stage::get().get_npcs().get_npcs();
		marker_sprite = Animation(marker["npc"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto npc = npcs->begin(); npc != npcs->end(); ++npc)
		{
			Point<int16_t> npc_pos = npc->second.get()->get_position();
			marker_sprite.draw((npc_pos + center_offset) / scale - sprite_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
		}

		/// Other characters
		MapObjects* chars = Stage::get().get_chars().get_chars();
		marker_sprite = Animation(marker["another"]);
		sprite_offset = marker_sprite.get_dimensions() / Point<int16_t>(2, 0);

		for (auto chr = chars->begin(); chr != chars->end(); ++chr)
		{
			Point<int16_t> chr_pos = chr->second.get()->get_position();
			marker_sprite.draw((chr_pos + center_offset) / scale - sprite_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
		}

		/// Player
		Point<int16_t> player_pos = Stage::get().get_player().get_position();
		sprite_offset = player_marker.get_dimensions() / Point<int16_t>(2, 0);
		player_marker.draw((player_pos + center_offset) / scale - sprite_offset + Point<int16_t>(map_draw_origin_x, map_draw_origin_y) + init_pos, alpha);
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