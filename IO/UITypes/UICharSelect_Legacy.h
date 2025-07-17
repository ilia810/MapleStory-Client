//////////////////////////////////////////////////////////////////////////////////
//	UICharSelect_Legacy - v83/v87 Character Selection using ViewAllChar
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../Components/MapleButton.h"
#include "../Components/AreaButton.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include "../../Character/Look/CharLook.h"
#include "../../Character/CharStats.h"
#include "../../Net/Login.h"
#include "../../Util/LegacyUI.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	class UICharSelect_Legacy : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHARSELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UICharSelect_Legacy(std::vector<CharEntry> characters, int8_t characters_count, int32_t slots, int8_t require_pic);

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void add_character(CharEntry&& character);
		void remove_character(int32_t id);
		const CharEntry& get_character(int32_t id);

		void send_naming_result(bool nameused);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		void update_buttons();
		void update_selected_character();
		void select_character_slot(int32_t slot);
		std::string get_slot_text() const;
		Point<int16_t> get_character_slot_pos(size_t index) const;

		enum Buttons : uint16_t
		{
			BtEnter,
			BtVAC,        // "View All Characters" - equivalent to BtNew
			BtDelete,
			BtBack,
			CHARACTER_SLOT0,
			CHARACTER_SLOT1,
			CHARACTER_SLOT2,
			CHARACTER_SLOT3,
			CHARACTER_SLOT4,
			CHARACTER_SLOT5,
			NUM_BUTTONS
		};

		// v83/v87 UI assets from ViewAllChar
		Text version;
		Point<int16_t> version_pos;
		
		Texture background;
		Texture world_icons;
		Texture character_slots[6];  // Select node has 6 frames for character slots
		
		// Character data
		std::vector<CharEntry> characters;
		std::vector<CharLook> charlooks;
		std::vector<Text> nametags;
		
		int8_t characters_count;
		int32_t slots;
		int8_t require_pic;
		int32_t selected_character;
		
		// Layout positions for v83/v87
		Point<int16_t> char_slot_positions[6];
		Point<int16_t> button_positions[5];  // Enter, VAC, Delete, Back + extra
		
		// Character slot bounds for click detection
		Rectangle<int16_t> char_slot_bounds[6];
		
		Text slot_label;
		Point<int16_t> slot_label_pos;
		
		bool char_look_valid[6];
	};
}