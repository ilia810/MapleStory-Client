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
#include "UIOptionMenu.h"

#include "../KeyAction.h"
#include "../UIScale.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIOptionMenu::UIOptionMenu() : UIDragElement<PosOPTIONMENU>(), selected_tab(0)
	{
		nl::node OptionMenu = nl::nx::UI["StatusBar3.img"]["OptionMenu"];
		nl::node backgrnd = OptionMenu["backgrnd"];

		sprites.emplace_back(backgrnd);
		sprites.emplace_back(OptionMenu["backgrnd2"]);

		nl::node graphic = OptionMenu["graphic"];

		tab_background[Buttons::TAB0] = graphic["layer:backgrnd"];
		tab_background[Buttons::TAB1] = OptionMenu["sound"]["layer:backgrnd"];
		tab_background[Buttons::TAB2] = OptionMenu["game"]["layer:backgrnd"];
		tab_background[Buttons::TAB3] = OptionMenu["invite"]["layer:backgrnd"];
		tab_background[Buttons::TAB4] = OptionMenu["screenshot"]["layer:backgrnd"];

		buttons[Buttons::CANCEL] = std::make_unique<MapleButton>(OptionMenu["button:Cancel"]);
		buttons[Buttons::OK] = std::make_unique<MapleButton>(OptionMenu["button:OK"]);
		buttons[Buttons::UIRESET] = std::make_unique<MapleButton>(OptionMenu["button:UIReset"]);

		nl::node tab = OptionMenu["tab"];
		nl::node tab_disabled = tab["disabled"];
		nl::node tab_enabled = tab["enabled"];

		for (size_t i = Buttons::TAB0; i < Buttons::CANCEL; i++)
			buttons[i] = std::make_unique<TwoSpriteButton>(tab_disabled[i], tab_enabled[i]);

		std::string sButtonUOL = graphic["combo:resolution"]["sButtonUOL"].get_string();
		std::string ctype = std::string(1, sButtonUOL.back());
		MapleComboBox::Type type = static_cast<MapleComboBox::Type>(std::stoi(ctype));

		std::vector<std::string> resolutions =
		{
			"800 x 600 ( 4 : 3 )",
			"1024 x 768 ( 4 : 3 )",
			"1280 x 720 ( 16 : 9 )",
			"1366 x 768 ( 16 : 9 )",
			"1920 x 1080 ( 16 : 9 ) - Beta"
		};

		int16_t max_width = Configuration::get().get_max_width();
		int16_t max_height = Configuration::get().get_max_height();

		if (max_width >= 1920 && max_height >= 1200)
			resolutions.emplace_back("1920 x 1200 ( 16 : 10 ) - Beta");

		uint16_t default_option = 0;
		int16_t screen_width = Constants::Constants::get().get_viewwidth();
		int16_t screen_height = Constants::Constants::get().get_viewheight();

		switch (screen_width)
		{
		case 800:
			default_option = 0;
			break;
		case 1024:
			default_option = 1;
			break;
		case 1280:
			default_option = 2;
			break;
		case 1366:
			default_option = 3;
			break;
		case 1920:
			switch (screen_height)
			{
			case 1080:
				default_option = 4;
				break;
			case 1200:
				default_option = 5;
				break;
			}

			break;
		}

		int64_t combobox_width = graphic["combo:resolution"]["boxWidth"].get_integer();
		Point<int16_t> lt = Point<int16_t>(graphic["combo:resolution"]["lt"]);

		buttons[Buttons::SELECT_RES] = std::make_unique<MapleComboBox>(type, resolutions, default_option, position, lt, combobox_width);

		// UI Scale combo box - placed below resolution
		std::vector<std::string> scale_options =
		{
			"75%",
			"100%",
			"125%",
			"150%",
			"175%",
			"200%"
		};

		// Determine current scale selection
		uint8_t current_scale = UIScale::get().get_scale_percent();
		uint16_t scale_default = 1; // Default to 100%
		switch (current_scale)
		{
		case 75: scale_default = 0; break;
		case 100: scale_default = 1; break;
		case 125: scale_default = 2; break;
		case 150: scale_default = 3; break;
		case 175: scale_default = 4; break;
		case 200: scale_default = 5; break;
		}

		// Position scale combo box below resolution (offset Y by ~25 pixels)
		Point<int16_t> scale_lt = lt + Point<int16_t>(0, 28);
		buttons[Buttons::SELECT_SCALE] = std::make_unique<MapleComboBox>(type, scale_options, scale_default, position, scale_lt, combobox_width);

		// Create label for UI Scale
		scale_label = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "UI Scale");

		Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

		dimension = bg_dimensions;
		dragarea = Point<int16_t>(bg_dimensions.x(), 20);

		change_tab(Buttons::TAB2);
	}

	void UIOptionMenu::draw(float inter) const
	{
		UIElement::draw_sprites(inter);

		tab_background[selected_tab].draw(position);

		// Draw scale label when on Graphics tab
		if (selected_tab == Buttons::TAB0)
		{
			// Draw label to the left of the combo box
			scale_label.draw(position + Point<int16_t>(25, 95));
		}

		UIElement::draw_buttons(inter);
	}

	Button::State UIOptionMenu::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case Buttons::TAB0:
		case Buttons::TAB1:
		case Buttons::TAB2:
		case Buttons::TAB3:
		case Buttons::TAB4:
			change_tab(buttonid);
			return Button::State::IDENTITY;
		case Buttons::CANCEL:
			deactivate();
			return Button::State::NORMAL;
		case Buttons::OK:
			switch (selected_tab)
			{
			case Buttons::TAB0:
			{
				// Apply resolution setting
				uint16_t selected_value = buttons[Buttons::SELECT_RES]->get_selected();

				int16_t width = Constants::Constants::get().get_viewwidth();
				int16_t height = Constants::Constants::get().get_viewheight();

				switch (selected_value)
				{
				case 0:
					width = 800;
					height = 600;
					break;
				case 1:
					width = 1024;
					height = 768;
					break;
				case 2:
					width = 1280;
					height = 720;
					break;
				case 3:
					width = 1366;
					height = 768;
					break;
				case 4:
					width = 1920;
					height = 1080;
					break;
				case 5:
					width = 1920;
					height = 1200;
					break;
				}

				Setting<Width>::get().save(width);
				Setting<Height>::get().save(height);

				Constants::Constants::get().set_viewwidth(width);
				Constants::Constants::get().set_viewheight(height);

				// Apply UI scale setting
				uint16_t scale_selected = buttons[Buttons::SELECT_SCALE]->get_selected();
				uint8_t scale_percent = 100;
				switch (scale_selected)
				{
				case 0: scale_percent = 75; break;
				case 1: scale_percent = 100; break;
				case 2: scale_percent = 125; break;
				case 3: scale_percent = 150; break;
				case 4: scale_percent = 175; break;
				case 5: scale_percent = 200; break;
				}
				UIScale::get().set_scale_percent(scale_percent);
				Setting<UIScaleSetting>::get().save(scale_percent);
			}
			break;
			case Buttons::TAB1:
			case Buttons::TAB2:
			case Buttons::TAB3:
			case Buttons::TAB4:
			default:
				break;
			}

			deactivate();
			return Button::State::NORMAL;
		case Buttons::UIRESET:
			return Button::State::DISABLED;
		case Buttons::SELECT_RES:
			buttons[Buttons::SELECT_RES]->toggle_pressed();
			return Button::State::NORMAL;
		case Buttons::SELECT_SCALE:
			buttons[Buttons::SELECT_SCALE]->toggle_pressed();
			return Button::State::NORMAL;
		default:
			return Button::State::DISABLED;
		}
	}

	Cursor::State UIOptionMenu::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

		if (dragged)
			return dstate;

		// Handle resolution combo box
		auto& res_button = buttons[Buttons::SELECT_RES];
		if (res_button->is_pressed())
		{
			if (res_button->in_combobox(cursorpos))
			{
				if (Cursor::State new_state = res_button->send_cursor(clicked, cursorpos))
					return new_state;
			}
			else
			{
				remove_cursor();
			}
		}

		// Handle scale combo box
		auto& scale_button = buttons[Buttons::SELECT_SCALE];
		if (scale_button->is_pressed())
		{
			if (scale_button->in_combobox(cursorpos))
			{
				if (Cursor::State new_state = scale_button->send_cursor(clicked, cursorpos))
					return new_state;
			}
			else
			{
				remove_cursor();
			}
		}

		return UIElement::send_cursor(clicked, cursorpos);
	}

	void UIOptionMenu::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (escape)
				deactivate();
			else if (keycode == KeyAction::Id::RETURN)
				button_pressed(Buttons::OK);
		}
	}

	UIElement::Type UIOptionMenu::get_type() const
	{
		return TYPE;
	}

	void UIOptionMenu::change_tab(uint16_t tabid)
	{
		buttons[selected_tab]->set_state(Button::State::NORMAL);
		buttons[tabid]->set_state(Button::State::PRESSED);

		selected_tab = tabid;

		switch (tabid)
		{
		case Buttons::TAB0:
			buttons[Buttons::SELECT_RES]->set_active(true);
			buttons[Buttons::SELECT_SCALE]->set_active(true);
			break;
		case Buttons::TAB1:
		case Buttons::TAB2:
		case Buttons::TAB3:
		case Buttons::TAB4:
			buttons[Buttons::SELECT_RES]->set_active(false);
			buttons[Buttons::SELECT_SCALE]->set_active(false);
			break;
		default:
			break;
		}
	}
}