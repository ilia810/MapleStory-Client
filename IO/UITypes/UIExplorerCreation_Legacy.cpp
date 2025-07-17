//////////////////////////////////////////////////////////////////////////////////
//	UIExplorerCreation_Legacy - v83/v87 Explorer character creation
//////////////////////////////////////////////////////////////////////////////////
#include "UIExplorerCreation_Legacy.h"

#include "UICharSelect_Legacy.h"
#include "UIRaceSelect.h"
#include "UILoginNotice.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Configuration.h"
#include "../../Audio/Audio.h"
#include "../../Net/Packets/CharCreationPackets.h"
#include "../../Character/Job.h"
#include "../KeyAction.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include <random>
#include <algorithm>
#include <cctype>

namespace ms
{
	UIExplorerCreation_Legacy::UIExplorerCreation_Legacy() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)),
		character_name(""),
		name_input_active(false),
		female(false),
		face(20000),
		hair(30000),
		skin(0),
		char_look_valid(false)
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
		
		// Load UI textures from v83/v87 assets
		nl::node NewChar = Login["NewChar"];
		if (NewChar) {
			name_input_bg = LegacyUI::get_or_dummy(NewChar["charName"], "Login.img/NewChar/charName");
			stat_dice_button = LegacyUI::get_or_dummy(NewChar["BtStat"], "Login.img/NewChar/BtStat");
			ok_button = LegacyUI::get_or_dummy(NewChar["BtOK"], "Login.img/NewChar/BtOK");
			cancel_button = LegacyUI::get_or_dummy(NewChar["BtCancel"], "Login.img/NewChar/BtCancel");
		}
		
		// Try to load Explorer job banner
		if (NewChar && NewChar["job"]) {
			job_banner = LegacyUI::get_or_dummy(NewChar["job"]["0"], "Login.img/NewChar/job/0");
		}
		
		// Create buttons using LegacyUI helpers
		if (NewChar) {
			if (auto bt_ok = LegacyUI::make_simple3(NewChar["BtOK"], LegacyUI::Layout::ExplorerCreation::OK_BUTTON_POS)) {
				buttons[Buttons::BtOK] = std::move(bt_ok);
			}
			if (auto bt_cancel = LegacyUI::make_simple3(NewChar["BtCancel"], LegacyUI::Layout::ExplorerCreation::CANCEL_BUTTON_POS)) {
				buttons[Buttons::BtCancel] = std::move(bt_cancel);
			}
			if (auto bt_stat = LegacyUI::make_simple3(NewChar["BtStat"], LegacyUI::Layout::ExplorerCreation::STAT_DICE_POS)) {
				buttons[Buttons::BtStatDice] = std::move(bt_stat);
			}
		}
		
		// Initialize character stats (Explorer starting stats)
		base_stats[MapleStat::Id::STR] = 12;
		base_stats[MapleStat::Id::DEX] = 5;
		base_stats[MapleStat::Id::INT] = 4;
		base_stats[MapleStat::Id::LUK] = 4;
		
		// Randomize stats initially
		randomize_stats();
		
		// Create stat display texts
		str_display = LegacyUI::create_stat_text(base_stats[MapleStat::Id::STR]);
		dex_display = LegacyUI::create_stat_text(base_stats[MapleStat::Id::DEX]);
		int_display = LegacyUI::create_stat_text(base_stats[MapleStat::Id::INT]);
		luk_display = LegacyUI::create_stat_text(base_stats[MapleStat::Id::LUK]);
		
		// Create name input display
		name_display = LegacyUI::create_input_text(character_name);
		
		// Initialize available appearance options for v83/v87
		// These are typical v83 starting options
		available_faces_m = {20000, 20001, 20002, 20012};
		available_faces_f = {21000, 21001, 21002, 21012};
		available_hairs_m = {30000, 30020, 30030};
		available_hairs_f = {31000, 31040, 31050};
		available_skins = {0, 1, 2, 3, 4}; // Different skin tones
		
		// Randomize initial appearance
		randomize_look();
		update_character_display();
		
		// Play creation sound
		LegacyUI::play_sound_safe(LegacyUI::LegacySound::UI_OPEN);
		
	}

	void UIExplorerCreation_Legacy::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw version text
		version.draw(position + version_pos);

		// Draw job banner
		if (job_banner.is_valid()) {
			job_banner.draw(position + LegacyUI::Layout::ExplorerCreation::JOB_BANNER_POS);
		}

		// Draw name input background
		if (name_input_bg.is_valid()) {
			name_input_bg.draw(position + LegacyUI::Layout::ExplorerCreation::NAME_INPUT_POS);
		}

		// Draw character name
		name_display.draw(position + LegacyUI::Layout::ExplorerCreation::NAME_INPUT_POS + Point<int16_t>(10, 5));

		// Draw stat displays
		str_display.draw(position + LegacyUI::Layout::ExplorerCreation::STR_POS);
		dex_display.draw(position + LegacyUI::Layout::ExplorerCreation::DEX_POS);
		int_display.draw(position + LegacyUI::Layout::ExplorerCreation::INT_POS);
		luk_display.draw(position + LegacyUI::Layout::ExplorerCreation::LUK_POS);

		// Draw character preview if available
		// TODO: Implement character rendering when CharLook system is working

		UIElement::draw_buttons(inter);
	}

	void UIExplorerCreation_Legacy::update()
	{
		UIElement::update();
		
		// Update text displays
		str_display.change_text(std::to_string(base_stats[MapleStat::Id::STR]));
		dex_display.change_text(std::to_string(base_stats[MapleStat::Id::DEX]));
		int_display.change_text(std::to_string(base_stats[MapleStat::Id::INT]));
		luk_display.change_text(std::to_string(base_stats[MapleStat::Id::LUK]));
		
		name_display.change_text(character_name);
	}

	Cursor::State UIExplorerCreation_Legacy::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Handle name input field click
		Rectangle<int16_t> name_input_bounds(
			position + LegacyUI::Layout::ExplorerCreation::NAME_INPUT_POS,
			position + LegacyUI::Layout::ExplorerCreation::NAME_INPUT_POS + Point<int16_t>(200, 30)
		);
		
		if (name_input_bounds.contains(cursorpos)) {
			if (clicked) {
				name_input_active = true;
				LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_CLICK);
			}
			return Cursor::State::CANCLICK;
		}

		// Handle button interactions
		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_OVER);
					btit.second->set_state(Button::State::MOUSEOVER);
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicked)
					{
						LegacyUI::play_sound_safe(LegacyUI::LegacySound::BUTTON_CLICK);
						btit.second->set_state(button_pressed(btit.first));
					}
				}
				return Cursor::State::CANCLICK;
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return Cursor::State::LEAF;
	}

	void UIExplorerCreation_Legacy::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				return_to_raceselect();
			}
			else if (name_input_active)
			{
				// Handle name input
				if (keycode == KeyAction::Id::RETURN)
				{
					name_input_active = false;
				}
				else if (keycode == KeyAction::Id::BACK && !character_name.empty())
				{
					character_name.pop_back();
				}
				else if (keycode >= 32 && keycode <= 126 && character_name.length() < 12)
				{
					// Add printable characters
					if (std::isalnum(keycode))
					{
						character_name += static_cast<char>(keycode);
					}
				}
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				button_pressed(Buttons::BtOK);
			}
			else if (keycode == KeyAction::Id::SPACE)
			{
				button_pressed(Buttons::BtStatDice);
			}
		}
	}

	UIElement::Type UIExplorerCreation_Legacy::get_type() const
	{
		return TYPE;
	}

	void UIExplorerCreation_Legacy::send_naming_result(bool nameused)
	{
		if (nameused)
		{
			// Name is already taken, show error
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::NAME_IN_USE);
		}
		else
		{
			// Name is available, continue with character creation
		}
	}

	Button::State UIExplorerCreation_Legacy::button_pressed(uint16_t buttonid)
	{
		if (buttonid == Buttons::BtOK)
		{
			create_character();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::BtCancel)
		{
			return_to_raceselect();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::BtStatDice)
		{
			randomize_stats();
			LegacyUI::play_sound_safe(LegacyUI::LegacySound::DICE_ROLL);
			return Button::State::NORMAL;
		}
		
		return Button::State::DISABLED;
	}

	void UIExplorerCreation_Legacy::randomize_stats()
	{
		// v83 Explorer stat randomization (total of 25 points)
		std::random_device rd;
		std::mt19937 gen(rd());
		
		// Distribute 25 points among 4 stats, with minimum values
		std::uniform_int_distribution<> stat_dist(0, 5);
		
		int remaining_points = 25 - 12 - 5 - 4 - 4; // Total minus minimums
		
		base_stats[MapleStat::Id::STR] = 12 + stat_dist(gen);
		remaining_points -= (base_stats[MapleStat::Id::STR] - 12);
		
		if (remaining_points > 0) {
			base_stats[MapleStat::Id::DEX] = 5 + std::min(remaining_points, stat_dist(gen));
			remaining_points -= (base_stats[MapleStat::Id::DEX] - 5);
		} else {
			base_stats[MapleStat::Id::DEX] = 5;
		}
		
		if (remaining_points > 0) {
			base_stats[MapleStat::Id::INT] = 4 + std::min(remaining_points, stat_dist(gen));
			remaining_points -= (base_stats[MapleStat::Id::INT] - 4);
		} else {
			base_stats[MapleStat::Id::INT] = 4;
		}
		
		base_stats[MapleStat::Id::LUK] = 4 + std::max(0, remaining_points);
		
	}

	void UIExplorerCreation_Legacy::randomize_look()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		
		// Randomize gender
		std::uniform_int_distribution<> gender_dist(0, 1);
		female = gender_dist(gen) == 1;
		
		// Randomize face
		if (female && !available_faces_f.empty()) {
			std::uniform_int_distribution<> face_dist(0, available_faces_f.size() - 1);
			face = available_faces_f[face_dist(gen)];
		} else if (!available_faces_m.empty()) {
			std::uniform_int_distribution<> face_dist(0, available_faces_m.size() - 1);
			face = available_faces_m[face_dist(gen)];
		}
		
		// Randomize hair
		if (female && !available_hairs_f.empty()) {
			std::uniform_int_distribution<> hair_dist(0, available_hairs_f.size() - 1);
			hair = available_hairs_f[hair_dist(gen)];
		} else if (!available_hairs_m.empty()) {
			std::uniform_int_distribution<> hair_dist(0, available_hairs_m.size() - 1);
			hair = available_hairs_m[hair_dist(gen)];
		}
		
		// Randomize skin
		if (!available_skins.empty()) {
			std::uniform_int_distribution<> skin_dist(0, available_skins.size() - 1);
			skin = available_skins[skin_dist(gen)];
		}
		
	}

	void UIExplorerCreation_Legacy::update_character_display()
	{
		// TODO: Create CharLook object and update character preview
		// This would require the CharLook system to be working properly
		char_look_valid = false;
	}

	void UIExplorerCreation_Legacy::create_character()
	{
		// Validate character name
		if (character_name.empty() || !LegacyUI::is_valid_character_name(character_name))
		{
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::ILLEGAL_NAME);
			return;
		}
		
		
		// Create character creation packet
		try {
			// Basic Explorer equipment IDs for v83/v87
			int32_t job = 0; // Beginner
			int32_t top = 1040036; // Basic shirt
			int32_t bottom = 1060026; // Basic pants  
			int32_t shoes = 1072001; // Basic shoes
			int32_t weapon = 1302000; // Basic sword
			
			// Send character creation packet
			CreateCharPacket(character_name, job, face, hair, skin, top, bottom, shoes, weapon, female).dispatch();
			
			// Return to character select
			deactivate();
			if (auto charselect = UI::get().get_element<UICharSelect_Legacy>()) {
				charselect->makeactive();
			}
		}
		catch (const std::exception& e) {
			UI::get().emplace<UILoginNotice>(UILoginNotice::Message::UNKNOWN_ERROR);
		}
	}

	void UIExplorerCreation_Legacy::return_to_raceselect()
	{
		LegacyUI::play_sound_safe(LegacyUI::LegacySound::UI_CLOSE);
		
		deactivate();
		UI::get().remove(UIElement::Type::CLASSCREATION);
		UI::get().emplace<UIRaceSelect>();
	}
}