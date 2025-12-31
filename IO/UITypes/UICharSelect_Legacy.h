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
		void select_character(int32_t index);

		enum Buttons : uint16_t
		{
			BtEnter,
			CHARACTER_SLOT0,
			NUM_BUTTONS
		};

		// Frame (stretched to 808x608)
		Texture frame;
		Point<int16_t> frame_stretch;

		// Character slot texture from ViewAllChar/Select
		Texture char_slot;
		Point<int16_t> char_slot_pos;
		Rectangle<int16_t> char_slot_bounds;

		// Character data
		std::vector<CharEntry> characters;
		std::vector<CharLook> charlooks;
		std::vector<Text> nametags;

		int8_t characters_count;
		int32_t slots;
		int8_t require_pic;
		int32_t selected_character;
	};
}
