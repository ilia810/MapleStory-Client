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
#include "../../Util/Misc.h"

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

		// Create version text
		version = LegacyUI::create_version_text();

		// Try to get version position from UI, fallback to default
		nl::node Login = nl::nx::UI["Login.img"];
		if (Login && Login["Common"] && Login["Common"]["version"] && Login["Common"]["version"]["pos"]) {
			version_pos = Login["Common"]["version"]["pos"];
		} else {
			version_pos = Point<int16_t>(10, 580); // Default bottom-left
		}

		// Load ViewAllChar assets (v83/v87 character selection)
		nl::node ViewAllChar = Login["ViewAllChar"];
		if (!ViewAllChar) {
			throw std::runtime_error("ViewAllChar assets not found in v83/v87 data");
		}


		// Load background
		background = LegacyUI::get_or_dummy(ViewAllChar["backgrnd"], "Login.img/ViewAllChar/backgrnd");

		// Load world icons
		world_icons = LegacyUI::get_or_dummy(ViewAllChar["WorldIcons"], "Login.img/ViewAllChar/WorldIcons");

		// Load character selection frames (Select node has 6 animation frames for character slots)
		nl::node Select = ViewAllChar["Select"];
		if (Select) {
			for (int i = 0; i < 6; i++) {
				character_slots[i] = LegacyUI::get_or_dummy(Select[std::to_string(i)], 
					"Login.img/ViewAllChar/Select/" + std::to_string(i));
			}
		}

		// Create buttons using v83/v87 ViewAllChar structure
		if (auto bt_enter = LegacyUI::make_simple3(ViewAllChar["BtEnter"], Point<int16_t>(600, 500))) {
			buttons[Buttons::BtEnter] = std::move(bt_enter);
		} else {
		}
		if (auto bt_vac = LegacyUI::make_simple3(ViewAllChar["BtVAC"], Point<int16_t>(200, 500))) {
			buttons[Buttons::BtVAC] = std::move(bt_vac);
		} else {
		}

		// For delete button, try Common area first, then fallback
		nl::node Common = Login["Common"];
		if (auto bt_delete = LegacyUI::make_simple3(Common["BtDelete"], Point<int16_t>(400, 500))) {
			buttons[Buttons::BtDelete] = std::move(bt_delete);
		} else {
		}
		if (auto bt_back = LegacyUI::make_simple3(Common["BtBack"], Point<int16_t>(100, 500))) {
			buttons[Buttons::BtBack] = std::move(bt_back);
		} else {
		}

		// Set up character slot positions for v83/v87 layout
		// Typical v83 character selection has 3x2 grid layout
		for (int i = 0; i < 6; i++) {
			int row = i / 3;
			int col = i % 3;
			char_slot_positions[i] = Point<int16_t>(150 + col * 200, 150 + row * 200);
			
			// Create clickable area for each character slot
			char_slot_bounds[i] = Rectangle<int16_t>(
				char_slot_positions[i] - Point<int16_t>(50, 75),  // Top-left
				char_slot_positions[i] + Point<int16_t>(50, 75)   // Bottom-right
			);
			
			// Create slot button
			buttons[Buttons::CHARACTER_SLOT0 + i] = std::make_unique<AreaButton>(
				char_slot_positions[i] - Point<int16_t>(50, 75), 
				Point<int16_t>(100, 150)
			);
		}

		// Initialize character looks and nametags
		charlooks.reserve(characters.size());
		nametags.reserve(characters.size());
		
		for (size_t i = 0; i < characters.size() && i < 6; i++) {
			try {
				charlooks.emplace_back(characters[i].look);
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, characters[i].stats.name);
				char_look_valid[i] = true;
			} catch (...) {
				charlooks.emplace_back();
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, "ERROR");
				char_look_valid[i] = false;
			}
		}

		// Fill remaining slots with empty data
		for (size_t i = characters.size(); i < 6; i++) {
			charlooks.emplace_back();
			nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::GRAY, "Empty");
			char_look_valid[i] = false;
		}

		// Create slot count label
		slot_label = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, get_slot_text());
		slot_label_pos = Point<int16_t>(50, 50);

		// Default to first character if any exist
		if (characters_count > 0) {
			selected_character = 0;
		}

		update_buttons();

		// Play character select sound
		LegacyUI::play_sound_safe(Sound::Name::CHARSELECT);

	}

	void UICharSelect_Legacy::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw version text
		version.draw(position + version_pos);

		// Draw background
		if (background.is_valid()) {
			background.draw(position);
		}

		// Draw world icons
		if (world_icons.is_valid()) {
			world_icons.draw(position + Point<int16_t>(50, 100));
		}

		// Draw character slots
		for (int i = 0; i < 6; i++) {
			Point<int16_t> slot_pos = position + char_slot_positions[i];
			
			// Draw slot background
			if (character_slots[i].is_valid()) {
				character_slots[i].draw(slot_pos - Point<int16_t>(50, 75));
			} else {
				// Fallback: draw a simple rectangle indicator
				LegacyUI::draw_fallback_rect(slot_pos - Point<int16_t>(50, 75), Point<int16_t>(100, 150), Color::Name::GRAY);
			}

			// Draw character if present
			if (i < charlooks.size() && char_look_valid[i] && i < characters.size()) {
				try {
					// Draw character look (simplified - just draw at slot position)
					// TODO: Implement proper character rendering when CharLook system is stable
					
					// Draw character name
					if (i < nametags.size()) {
						nametags[i].draw(slot_pos + Point<int16_t>(0, 60));
					}

					// Highlight selected character
					if (i == selected_character) {
						// TODO: Draw selection highlight
					}
				} catch (...) {
					// Silent fallback for character rendering errors
				}
			}
		}

		// Draw slot count label
		slot_label.draw(position + slot_label_pos);

		UIElement::draw_buttons(inter);
	}

	void UICharSelect_Legacy::update()
	{
		UIElement::update();
		
		// Update slot count text
		slot_label.change_text(get_slot_text());
	}

	Cursor::State UICharSelect_Legacy::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Check character slot clicks
		for (int i = 0; i < 6; i++) {
			if (char_slot_bounds[i].contains(cursorpos)) {
				if (clicked) {
					select_character_slot(i);
					LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_CLICK);
				}
				return Cursor::State::CANCLICK;
			}
		}

		// Handle button interactions
		for (auto& btit : buttons) {
			if (btit.second && btit.second->is_active() && btit.second->bounds(position).contains(cursorpos)) {
				if (btit.second->get_state() == Button::State::NORMAL) {
					LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_OVER);
					btit.second->set_state(Button::State::MOUSEOVER);
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER) {
					if (clicked) {
						LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_CLICK);
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
			else if (keycode >= '1' && keycode <= '6') {
				// Select character slot by number key
				int slot = keycode - '1';
				select_character_slot(slot);
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
				char_look_valid[characters.size() - 1] = true;
			} catch (...) {
				charlooks.emplace_back();
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::WHITE, "ERROR");
				char_look_valid[characters.size() - 1] = false;
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
				
				// Add empty slot
				charlooks.emplace_back();
				nametags.emplace_back(Text::Font::A13M, Text::Alignment::CENTER, Color::Name::GRAY, "Empty");
				char_look_valid[charlooks.size() - 1] = false;
				
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
			} else {
			}
			return Button::State::DISABLED;

		case Buttons::BtVAC:
			// "View All Characters" - equivalent to "New Character"
			deactivate();
			UI::get().emplace<UIRaceSelect>();
			return Button::State::NORMAL;

		case Buttons::BtDelete:
			if (selected_character >= 0 && selected_character < characters_count) {
				// TODO: Implement character deletion packet
				// DeleteCharPacket(characters[selected_character].id).dispatch();
			} else {
			}
			return Button::State::NORMAL;

		case Buttons::BtBack:
			UI::get().remove(UIElement::Type::CHARSELECT);
			UI::get().emplace<UIWorldSelect>();
			return Button::State::NORMAL;

		default:
			if (buttonid >= Buttons::CHARACTER_SLOT0 && buttonid < Buttons::CHARACTER_SLOT0 + 6) {
				int slot = buttonid - Buttons::CHARACTER_SLOT0;
				select_character_slot(slot);
			}
			return Button::State::NORMAL;
		}
	}

	void UICharSelect_Legacy::update_buttons()
	{
		// Enable/disable buttons based on current state
		bool character_selected = (selected_character >= 0 && selected_character < characters_count);
		
		if (buttons[Buttons::BtEnter]) {
			buttons[Buttons::BtEnter]->set_state(character_selected ? Button::State::NORMAL : Button::State::DISABLED);
		}
		if (buttons[Buttons::BtDelete]) {
			buttons[Buttons::BtDelete]->set_state(character_selected ? Button::State::NORMAL : Button::State::DISABLED);
		}

		// Update character slot buttons
		for (int i = 0; i < 6; i++) {
			if (buttons[Buttons::CHARACTER_SLOT0 + i]) {
				bool has_character = (i < characters_count);
				buttons[Buttons::CHARACTER_SLOT0 + i]->set_state(has_character ? Button::State::NORMAL : Button::State::DISABLED);
			}
		}
	}

	void UICharSelect_Legacy::update_selected_character()
	{
		if (selected_character >= 0 && selected_character < characters_count) {
			LegacyUI::play_sound_safe(Sound::Name::CHARSELECT);
		}
		update_buttons();
	}

	void UICharSelect_Legacy::select_character_slot(int32_t slot)
	{
		if (slot >= 0 && slot < 6 && slot < characters_count) {
			selected_character = slot;
			update_selected_character();
		} else if (slot >= 0 && slot < 6 && slot >= characters_count) {
			// Could open character creation here
		}
	}

	std::string UICharSelect_Legacy::get_slot_text() const
	{
		return std::to_string(characters_count) + "/" + std::to_string(slots);
	}

	Point<int16_t> UICharSelect_Legacy::get_character_slot_pos(size_t index) const
	{
		if (index < 6) {
			return char_slot_positions[index];
		}
		return Point<int16_t>(0, 0);
	}
}