//////////////////////////////////////////////////////////////////////////////////
//	UICharSelect_Legacy - v83/v87 Character Selection using ViewAllChar
//////////////////////////////////////////////////////////////////////////////////
#include "UICharSelect_Legacy.h"

#include "UILoginNotice.h"
#include "UIRaceSelect.h"
#include "UIWorldSelect.h"

#include "../UI.h"
#include "../../Configuration.h"
#include "../../Audio/Audio.h"
#include "../../Net/Packets/SelectCharPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UICharSelect_Legacy::UICharSelect_Legacy(std::vector<CharEntry> characters, int8_t characters_count, int32_t slots, int8_t require_pic)
		: UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)),
		characters(std::move(characters)), characters_count(characters_count), slots(slots), require_pic(require_pic),
		selected_character(-1)
	{
		nl::node Login = nl::nx::UI["Login.img"];
		nl::node Common = Login["Common"];
		nl::node ViewAllChar = Login["ViewAllChar"];

		// Frame/border from Common - stretched to fill 800x600
		if (Common["frame"]) {
			frame = Texture(Common["frame"]);
			frame_stretch = Point<int16_t>(808, 608);
		}

		// Character slot from ViewAllChar/Select at (81, 180)
		if (ViewAllChar["Select"]) {
			char_slot = Texture(ViewAllChar["Select"]);
			char_slot_pos = Point<int16_t>(81, 180);

			// Create clickable area for character slot (126x125)
			char_slot_bounds = Rectangle<int16_t>(
				char_slot_pos,
				char_slot_pos + Point<int16_t>(126, 125)
			);

			buttons[Buttons::CHARACTER_SLOT0] = std::make_unique<AreaButton>(
				char_slot_pos,
				Point<int16_t>(126, 125)
			);
		}

		// BtEnter button at (616, 164)
		if (ViewAllChar["BtEnter"]) {
			buttons[Buttons::BtEnter] = std::make_unique<MapleButton>(ViewAllChar["BtEnter"], Point<int16_t>(616, 164));
		}

		// Initialize character looks and nametags
		charlooks.reserve(this->characters.size());
		nametags.reserve(this->characters.size());

		for (size_t i = 0; i < this->characters.size(); i++) {
			try {
				charlooks.emplace_back(this->characters[i].look);
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, this->characters[i].stats.name);
			} catch (...) {
				charlooks.emplace_back();
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, "ERROR");
			}
		}

		// Default to first character if any exist
		if (characters_count > 0) {
			selected_character = 0;
		}

		update_buttons();

		// Play character select sound
		Sound(Sound::Name::CHARSELECT).play();
	}

	void UICharSelect_Legacy::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw frame stretched to fill 800x600
		if (frame.is_valid()) {
			frame.draw(DrawArgument(
				position + frame.get_origin(),
				Point<int16_t>(0, 0),
				frame_stretch,
				1.0f, 1.0f, 1.0f, 0.0f
			));
		}

		// Draw character slot at (81, 180)
		if (char_slot.is_valid()) {
			char_slot.draw(position + char_slot_pos);
		}

		// Draw character name if character exists
		if (selected_character >= 0 && selected_character < static_cast<int32_t>(nametags.size())) {
			// Draw name centered below the slot
			nametags[selected_character].draw(position + char_slot_pos + Point<int16_t>(63, 130));
		}

		UIElement::draw_buttons(inter);
	}

	void UICharSelect_Legacy::update()
	{
		UIElement::update();
	}

	Cursor::State UICharSelect_Legacy::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Check character slot click
		if (char_slot_bounds.contains(cursorpos)) {
			if (clicked && characters_count > 0) {
				select_character(0);
				Sound(Sound::Name::BUTTONCLICK).play();
			}
			return Cursor::State::CANCLICK;
		}

		// Handle button interactions
		for (auto& btit : buttons) {
			if (btit.second && btit.second->is_active() && btit.second->bounds(position).contains(cursorpos)) {
				if (btit.second->get_state() == Button::State::NORMAL) {
					Sound(Sound::Name::BUTTONOVER).play();
					btit.second->set_state(Button::State::MOUSEOVER);
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER) {
					if (clicked) {
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(button_pressed(btit.first));
					}
				}
				return Cursor::State::CANCLICK;
			}
			else if (btit.second && btit.second->get_state() == Button::State::MOUSEOVER) {
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return Cursor::State::LEAF;
	}

	void UICharSelect_Legacy::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed) {
			if (escape) {
				// Return to world select
				UI::get().remove(UIElement::Type::CHARSELECT);
				UI::get().emplace<UIWorldSelect>();
			}
			else if (keycode == KeyAction::Id::RETURN) {
				// Enter game with selected character
				button_pressed(Buttons::BtEnter);
			}
		}
	}

	UIElement::Type UICharSelect_Legacy::get_type() const
	{
		return TYPE;
	}

	void UICharSelect_Legacy::add_character(CharEntry&& character)
	{
		if (characters.size() < 6) {
			characters.emplace_back(std::move(character));
			try {
				charlooks.emplace_back(characters.back().look);
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, characters.back().stats.name);
			} catch (...) {
				charlooks.emplace_back();
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, "ERROR");
			}
			characters_count++;
			update_buttons();
		}
	}

	void UICharSelect_Legacy::remove_character(int32_t id)
	{
		for (size_t i = 0; i < characters.size(); i++) {
			if (characters[i].id == id) {
				characters.erase(characters.begin() + i);
				if (i < charlooks.size()) charlooks.erase(charlooks.begin() + i);
				if (i < nametags.size()) nametags.erase(nametags.begin() + i);

				characters_count--;
				if (selected_character >= characters_count) {
					selected_character = characters_count > 0 ? 0 : -1;
				}
				update_buttons();
				break;
			}
		}
	}

	const CharEntry& UICharSelect_Legacy::get_character(int32_t id)
	{
		for (const CharEntry& character : characters) {
			if (character.id == id) {
				return character;
			}
		}
		static CharEntry null_character;
		return null_character;
	}

	void UICharSelect_Legacy::send_naming_result(bool nameused)
	{
		if (nameused) {
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::NAME_IN_USE);
		}
	}

	Button::State UICharSelect_Legacy::button_pressed(uint16_t buttonid)
	{
		switch (buttonid) {
		case Buttons::BtEnter:
			if (selected_character >= 0 && selected_character < characters_count) {
				SelectCharPacket(characters[selected_character].id).dispatch();

				// Disable the button to prevent multiple clicks
				if (buttons[Buttons::BtEnter]) {
					buttons[Buttons::BtEnter]->set_state(Button::State::DISABLED);
				}

				// Deactivate this UI since we're transitioning to game
				deactivate();
			}
			return Button::State::DISABLED;

		case Buttons::CHARACTER_SLOT0:
			select_character(0);
			return Button::State::NORMAL;

		default:
			return Button::State::NORMAL;
		}
	}

	void UICharSelect_Legacy::update_buttons()
	{
		// Enable/disable enter button based on selection
		bool character_selected = (selected_character >= 0 && selected_character < characters_count);

		if (buttons[Buttons::BtEnter]) {
			buttons[Buttons::BtEnter]->set_state(character_selected ? Button::State::NORMAL : Button::State::DISABLED);
		}
	}

	void UICharSelect_Legacy::select_character(int32_t index)
	{
		if (index >= 0 && index < characters_count) {
			selected_character = index;
			Sound(Sound::Name::CHARSELECT).play();
			update_buttons();
		}
	}
}
