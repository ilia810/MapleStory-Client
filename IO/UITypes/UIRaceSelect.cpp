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
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "UIRaceSelect.h"

#include "UIAranCreation.h"
#include "UICharSelect.h"
#include "UICygnusCreation.h"
#include "UIExplorerCreation.h"
#include "UIExplorerCreation_Legacy.h"
#include "UILoginNotice.h"
#include "UIWorldSelect.h"

#include "../UI.h"

#include "../Components/AreaButton.h"
#include "../Components/MapleButton.h"

#include "../../Configuration.h"

#include "../../Audio/Audio.h"
#include "../../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIRaceSelect::UIRaceSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600))
	{
		
		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);
		version_pos = nl::nx::UI["Login.img"]["Common"]["version"]["pos"];

		nl::node Login = nl::nx::UI["Login.img"];
		nl::node Common = Login["Common"];
		nl::node RaceSelect = Login["RaceSelect"];


		// Load textures for Explorer (normal state = Explorer class)
		if (RaceSelect["normal"]) {
			explorerN = Texture(RaceSelect["normal"]["0"]);
			explorerH = Texture(RaceSelect["normal"]["1"]);
			explorerP = Texture(RaceSelect["normal"]["2"]);
		}

		// Load textures for Knight (knight state = Cygnus Knights)
		if (RaceSelect["knight"]) {
			knightN = Texture(RaceSelect["knight"]["0"]);
			knightH = Texture(RaceSelect["knight"]["1"]);
			knightP = Texture(RaceSelect["knight"]["2"]);
		}

		// Load textures for Aran (aran state = Aran class)
		if (RaceSelect["aran"]) {
			aranN = Texture(RaceSelect["aran"]["0"]);
			aranH = Texture(RaceSelect["aran"]["1"]);
			aranP = Texture(RaceSelect["aran"]["2"]);
		}

		// Set center positions for 800x600 screen
		Point<int16_t> centerExplorer(200, 300);
		Point<int16_t> centerKnight(400, 300);
		Point<int16_t> centerAran(600, 300);

		// Adjust positions based on texture origins
		if (explorerN.is_valid()) {
			explorerPos = centerExplorer - explorerN.get_origin();
		} else {
			explorerPos = centerExplorer;
		}

		if (knightN.is_valid()) {
			knightPos = centerKnight - knightN.get_origin();
		} else {
			knightPos = centerKnight;
		}

		if (aranN.is_valid()) {
			aranPos = centerAran - aranN.get_origin();
		} else {
			aranPos = centerAran;
		}

		// Create clickable areas for each class icon
		Point<int16_t> defaultDimensions(100, 100);
		
		if (explorerN.is_valid()) {
			buttons[Buttons::CLASS0] = std::make_unique<AreaButton>(explorerPos, explorerN.get_dimensions());
		} else {
			buttons[Buttons::CLASS0] = std::make_unique<AreaButton>(explorerPos, defaultDimensions);
		}

		if (knightN.is_valid()) {
			buttons[Buttons::CLASS1] = std::make_unique<AreaButton>(knightPos, knightN.get_dimensions());
		} else {
			buttons[Buttons::CLASS1] = std::make_unique<AreaButton>(knightPos, defaultDimensions);
		}

		if (aranN.is_valid()) {
			buttons[Buttons::CLASS2] = std::make_unique<AreaButton>(aranPos, aranN.get_dimensions());
		} else {
			buttons[Buttons::CLASS2] = std::make_unique<AreaButton>(aranPos, defaultDimensions);
		}

		// Create navigation buttons if available
		if (Common["BtStart"]) {
			buttons[Buttons::BtStart] = std::make_unique<MapleButton>(Common["BtStart"], Point<int16_t>(0, 1));
		}
		
		if (Common["BtPreview"]) {
			buttons[Buttons::BtPreview] = std::make_unique<MapleButton>(Common["BtPreview"]);
		}

		// Initialize states
		selected_class = 0; // Default to Explorer
		mouseover[0] = false;
		mouseover[1] = false;
		mouseover[2] = false;

		// Play race select sound
		Sound(Sound::Name::RACESELECT).play();
		
	}

	void UIRaceSelect::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		// Draw version text
		version.draw(position + version_pos - Point<int16_t>(0, 5));

		// Draw Explorer icon
		auto explorerBtn = buttons.find(Buttons::CLASS0);
		if (explorerBtn != buttons.end() && explorerBtn->second && explorerBtn->second->get_state() == Button::State::PRESSED) {
			if (explorerP.is_valid()) explorerP.draw(position + explorerPos);
		} else if (mouseover[0]) {
			if (explorerH.is_valid()) explorerH.draw(position + explorerPos);
		} else {
			if (explorerN.is_valid()) explorerN.draw(position + explorerPos);
		}

		// Draw Knight icon
		auto knightBtn = buttons.find(Buttons::CLASS1);
		if (knightBtn != buttons.end() && knightBtn->second && knightBtn->second->get_state() == Button::State::PRESSED) {
			if (knightP.is_valid()) knightP.draw(position + knightPos);
		} else if (mouseover[1]) {
			if (knightH.is_valid()) knightH.draw(position + knightPos);
		} else {
			if (knightN.is_valid()) knightN.draw(position + knightPos);
		}

		// Draw Aran icon
		auto aranBtn = buttons.find(Buttons::CLASS2);
		if (aranBtn != buttons.end() && aranBtn->second && aranBtn->second->get_state() == Button::State::PRESSED) {
			if (aranP.is_valid()) aranP.draw(position + aranPos);
		} else if (mouseover[2]) {
			if (aranH.is_valid()) aranH.draw(position + aranPos);
		} else {
			if (aranN.is_valid()) aranN.draw(position + aranPos);
		}

		UIElement::draw_buttons(inter);
	}

	void UIRaceSelect::update()
	{
		UIElement::update();
		// No animated sprites to update in v83/v87 RaceSelect UI
	}

	Cursor::State UIRaceSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();

					// Set mouseover state for class buttons
					if (btit.first == Buttons::CLASS0) mouseover[0] = true;
					else if (btit.first == Buttons::CLASS1) mouseover[1] = true;
					else if (btit.first == Buttons::CLASS2) mouseover[2] = true;

					btit.second->set_state(Button::State::MOUSEOVER);
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();
						btit.second->set_state(button_pressed(btit.first));
					}
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				// Clear mouseover state for class buttons
				if (btit.first == Buttons::CLASS0) mouseover[0] = false;
				else if (btit.first == Buttons::CLASS1) mouseover[1] = false;
				else if (btit.first == Buttons::CLASS2) mouseover[2] = false;

				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return Cursor::State::LEAF;
	}

	void UIRaceSelect::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
			{
				show_charselect();
			}
			else if (keycode == KeyAction::Id::RETURN)
			{
				// Default to Explorer creation on Enter
				button_pressed(Buttons::CLASS0);
			}
		}
	}

	UIElement::Type UIRaceSelect::get_type() const
	{
		return TYPE;
	}

	bool UIRaceSelect::check_name(std::string name) const
	{
		nl::node ForbiddenName = nl::nx::Etc["ForbiddenName.img"];

		for (std::string forbiddenName : ForbiddenName)
		{
			std::string lName = to_lower(name);
			std::string fName = to_lower(forbiddenName);

			if (lName.find(fName) != std::string::npos)
				return false;
		}

		return true;
	}

	void UIRaceSelect::send_naming_result(bool nameused)
	{
		// This method is called by character creation screens
		// Implementation depends on which creation screen is active
	}

	Button::State UIRaceSelect::button_pressed(uint16_t buttonid)
	{
		if (buttonid == Buttons::BtStart)
		{
			show_worldselect();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::BtPreview)
		{
			show_charselect();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::CLASS0)
		{
			// Explorer class selected
			Sound(Sound::Name::SCROLLUP).play();
			deactivate();
			UI::get().emplace<UIExplorerCreation_Legacy>();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::CLASS1)
		{
			// Cygnus Knight class selected
			Sound(Sound::Name::SCROLLUP).play();
			deactivate();
			UI::get().emplace<UICygnusCreation>();
			return Button::State::NORMAL;
		}
		else if (buttonid == Buttons::CLASS2)
		{
			// Aran class selected
			Sound(Sound::Name::SCROLLUP).play();
			deactivate();
			UI::get().emplace<UIAranCreation>();
			return Button::State::NORMAL;
		}
		else
		{
			return Button::State::DISABLED;
		}
	}

	void UIRaceSelect::show_charselect()
	{
		Sound(Sound::Name::SCROLLUP).play();

		UI::get().remove(UIElement::Type::RACESELECT);

		if (auto charselect = UI::get().get_element<UICharSelect>())
			charselect->makeactive();
	}

	void UIRaceSelect::show_worldselect()
	{
		Sound(Sound::Name::SCROLLUP).play();

		UI::get().remove(UIElement::Type::RACESELECT);
		UI::get().remove(UIElement::Type::CHARSELECT);

		if (auto worldselect = UI::get().get_element<UIWorldSelect>())
			worldselect->makeactive();
	}

	std::string UIRaceSelect::to_lower(std::string value) const
	{
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		return value;
	}
}