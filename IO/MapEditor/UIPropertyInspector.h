//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../Components/Textfield.h"
#include "../../Graphics/Text.h"
#include "MapFormat.h"
#include <memory>
#include <functional>

namespace ms
{
	class UIPropertyInspector : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAP_EDITOR_PROPERTIES;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		
		UIPropertyInspector();
		
		void draw(float inter) const override;
		void update() override;
		
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;
		UIElement::Type get_type() const override { return TYPE; }
		
		// Different object types that can be inspected
		enum InspectorMode
		{
			MODE_NONE,
			MODE_MAP,
			MODE_PORTAL,
			MODE_NPC,
			MODE_MOB,
			MODE_SPAWN
		};
		
		// Set what to inspect
		void set_map_properties(MapFormat* map);
		void set_portal(PortalData* portal);
		void set_npc(NpcData* npc);
		void set_mob_spawn(MobSpawnData* mob);
		void set_spawn_point(int16_t* x, int16_t* y);
		void clear_selection();
		
		// Callbacks for when properties change
		void set_on_change_callback(std::function<void()> callback) { on_change = callback; }
		
	protected:
		Button::State button_pressed(uint16_t buttonid) override;
		
	private:
		struct PropertyField
		{
			std::string label;
			std::unique_ptr<Textfield> field;
			Point<int16_t> position;
			std::function<std::string()> getter;
			std::function<void(const std::string&)> setter;
			bool numeric_only;
			
			PropertyField(const std::string& label, Point<int16_t> pos, int16_t width,
				std::function<std::string()> get, std::function<void(const std::string&)> set, bool numeric = false);
		};
		
		void init_map_properties();
		void init_portal_properties();
		void init_npc_properties();
		void init_mob_properties();
		void init_spawn_properties();
		
		void draw_section_header(const std::string& title, Point<int16_t> pos) const;
		void update_field_values();
		void apply_changes();
		void apply_field_changes(int16_t field_index);
		
		InspectorMode mode;
		std::vector<PropertyField> properties;
		int16_t selected_field;
		
		// References to edited objects
		MapFormat* current_map;
		PortalData* current_portal;
		NpcData* current_npc;
		MobSpawnData* current_mob;
		int16_t* spawn_x;
		int16_t* spawn_y;
		
		// Visual elements
		Text header_text;
		Text label_text;
		Color inspector_bg;
		Color section_bg;
		
		// Callback
		std::function<void()> on_change;
		
		// Constants
		static constexpr int16_t INSPECTOR_WIDTH = 300;
		static constexpr int16_t FIELD_HEIGHT = 25;
		static constexpr int16_t FIELD_SPACING = 30;
		static constexpr int16_t SECTION_PADDING = 10;
		
		enum Buttons : uint16_t
		{
			BT_APPLY = 0,
			BT_REVERT = 1
		};
	};
}