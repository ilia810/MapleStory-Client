//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "UIState.h"

namespace ms
{
	// The UI state for the map editor mode
	class UIStateMapEditor : public UIState
	{
	public:
		UIStateMapEditor();

		void draw(float inter, Point<int16_t> cursor) const override;
		void update() override;

		void doubleclick(Point<int16_t> pos) override;
		void rightclick(Point<int16_t> pos) override;
		void send_key(KeyType::Id type, int32_t action, bool down, bool escape) override;
		Cursor::State send_cursor(Point<int16_t> cursor_position, Cursor::State cursor_state) override;
		void send_scroll(double yoffset) override;
		void send_close() override;

		void drag_icon(Icon* icon) override;
		void clear_tooltip(Tooltip::Parent parent) override;
		void show_equip(Tooltip::Parent parent, int16_t slot) override;
		void show_item(Tooltip::Parent parent, int32_t itemid) override;
		void show_skill(Tooltip::Parent parent, int32_t skill_id, int32_t level, int32_t masterlevel, int64_t expiration) override;
		void show_text(Tooltip::Parent parent, std::string text) override;
		void show_map(Tooltip::Parent parent, std::string name, std::string description, int32_t mapid, bool bolded, bool portal) override;

		Iterator pre_add(UIElement::Type type, bool toggled, bool focused) override;
		void remove(UIElement::Type type) override;
		UIElement* get(UIElement::Type type) override;
		UIElement* get_front(std::list<UIElement::Type> types) override;
		UIElement* get_front(Point<int16_t> pos) override;

	private:
		void init_editor_ui();
		void load_default_map();
		void load_test_map();
		
		// File operations
		void new_map();
		void open_map();
		void save_map();
		void save_map_as();
		
		// Object selection
		void select_object_at(Point<int16_t> map_pos);
		void clear_selection();
		void delete_selected_object();
		
		// Play mode functionality
		void switch_to_play_mode();
		void switch_to_edit_mode();
		void handle_play_mode_input(int32_t action, bool pressed);
		void update_player_movement();
		void draw_player() const;
		
		// Asset management
		void add_asset_to_palette(int32_t asset_id);
		
		// Map editor specific members
		std::shared_ptr<class MapFormat> current_map;
		std::unique_ptr<class MapRenderer> map_renderer;
		std::string current_map_path;
		
		// Mouse state
		Point<int16_t> mouse_position;
		bool is_placing_tile;
		bool is_dragging;
		Point<int16_t> last_tile_position;
		
		// Selection state
		enum SelectionType
		{
			SEL_NONE,
			SEL_PORTAL,
			SEL_NPC,
			SEL_MOB,
			SEL_SPAWN
		};
		
		SelectionType selection_type;
		int32_t selected_index;
		
		// Play mode state
		enum EditorMode
		{
			MODE_EDIT,
			MODE_PLAY
		};
		
		EditorMode editor_mode;
		Point<int16_t> player_position;
		
		// Movement state for play mode
		struct MovementState
		{
			bool moving_left = false;
			bool moving_right = false;
			bool moving_up = false;
			bool moving_down = false;
		} movement_state;
		
		template <class T, typename... Args>
		void emplace(Args&&... args)
		{
			if (auto iter = pre_add(T::TYPE, T::TOGGLED, T::FOCUSED))
			{
				(*iter).second = std::make_unique<T>(std::forward<Args>(args)...);
			}
		}
		
		EnumMap<UIElement::Type, std::unique_ptr<UIElement>, UIElement::Type::NUM_TYPES> elements;
		std::list<UIElement::Type> elementorder;
		Keyboard keyboard;
		Optional<Tooltip> tooltip;
		Optional<Icon> dragged_icon;
	};
}