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
#include "UIInspector.h"

#include "../../Graphics/GraphicsGL.h"
#include "../../Constants.h"
#include "../../MapleStory.h"

#include <sstream>
#include <iostream>
#include <iomanip>

namespace ms
{
	// Static members for InspectorMode
	bool InspectorMode::enabled = false;
	UIElement* InspectorMode::hovered_element = nullptr;
	UIElement::Type InspectorMode::hovered_type = UIElement::Type::NONE;

	bool InspectorMode::is_enabled()
	{
		return enabled;
	}

	void InspectorMode::toggle()
	{
		enabled = !enabled;
		if (!enabled)
		{
			hovered_element = nullptr;
			hovered_type = UIElement::Type::NONE;
		}
	}

	void InspectorMode::enable()
	{
		enabled = true;
	}

	void InspectorMode::disable()
	{
		enabled = false;
		hovered_element = nullptr;
		hovered_type = UIElement::Type::NONE;
	}

	void InspectorMode::set_hovered_element(UIElement* element, UIElement::Type type)
	{
		hovered_element = element;
		hovered_type = type;
	}

	UIElement* InspectorMode::get_hovered_element()
	{
		return hovered_element;
	}

	UIElement::Type InspectorMode::get_hovered_type()
	{
		return hovered_type;
	}

	UIInspector::UIInspector() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(0, 0))
	{
		inspected_element = nullptr;
		inspected_type = UIElement::Type::NONE;
		info_position = Point<int16_t>(10, 10);
		cursor_pos = Point<int16_t>(0, 0);
		inspecting = false;
	}

	void UIInspector::draw(float alpha) const
	{
		if (!InspectorMode::is_enabled())
			return;

		// Draw mode indicator at top of screen
		Text mode_text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::YELLOW,
			"[UI INSPECTOR MODE - F12 to toggle, Click to inspect]");
		mode_text.draw(Point<int16_t>(10, 10));

		// Draw bounds of hovered element
		UIElement* hovered = InspectorMode::get_hovered_element();
		if (hovered)
		{
			draw_element_bounds();
		}

		// Draw info panel if we have an inspected element
		if (inspected_element)
		{
			draw_info_panel();
		}
	}

	void UIInspector::draw_element_bounds() const
	{
		UIElement* element = InspectorMode::get_hovered_element();
		if (!element)
			return;

		// Get element bounds - we need to access protected members
		// For now, we'll draw a simple rectangle at cursor position
		// This will be enhanced when we have access to element bounds

		// Draw crosshair at cursor
		int16_t x = cursor_pos.x();
		int16_t y = cursor_pos.y();

		// We'll use the GraphicsGL to draw lines
		// For now, just indicate with text
		std::stringstream ss;
		ss << "Hover: " << get_type_name(InspectorMode::get_hovered_type());
		Text hover_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, ss.str());
		hover_text.draw(Point<int16_t>(10, 30));
	}

	void UIInspector::draw_info_panel() const
	{
		if (!inspected_element)
			return;

		// Draw semi-transparent background panel
		int16_t panel_x = info_position.x();
		int16_t panel_y = info_position.y() + 50;
		int16_t panel_width = 280;
		int16_t panel_height = 20 + (int16_t)(info_lines.size() * 16);

		// Draw panel background using GraphicsGL
		GraphicsGL::get().drawrectangle(
			panel_x, panel_y, panel_width, panel_height,
			PANEL_BG_R, PANEL_BG_G, PANEL_BG_B, PANEL_BG_A
		);

		// Draw border
		GraphicsGL::get().drawrectangle(
			panel_x, panel_y, panel_width, 2,
			BOUNDS_COLOR_R, BOUNDS_COLOR_G, BOUNDS_COLOR_B, BOUNDS_COLOR_A
		);

		// Draw info text
		int16_t text_y = panel_y + 5;
		for (const auto& line : info_lines)
		{
			line.draw(Point<int16_t>(panel_x + 5, text_y));
			text_y += 16;
		}
	}

	void UIInspector::update()
	{
		// Update info text if inspecting
		if (inspected_element)
		{
			update_info_text();
		}
	}

	void UIInspector::update_info_text()
	{
		info_lines.clear();

		if (!inspected_element)
			return;

		// Type name
		std::stringstream ss;
		ss << "Type: " << get_type_name(inspected_type);
		info_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, ss.str());

		// Position - access through public interface or estimate
		ss.str("");
		ss << "Active: " << (inspected_element->is_active() ? "Yes" : "No");
		info_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, ss.str());

		// We need to get position/dimension from the element
		// Since they're protected, we'll add accessor methods or use inspection
		ss.str("");
		ss << "Click position: (" << cursor_pos.x() << ", " << cursor_pos.y() << ")";
		info_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::LIGHTGREY, ss.str());

		// Add hint about element
		ss.str("");
		ss << "---";
		info_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DARKGREY, ss.str());

		ss.str("");
		ss << "Tip: Check UITypes/" << get_type_name(inspected_type) << ".cpp";
		info_lines.emplace_back(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::ORANGE, ss.str());
	}

	Cursor::State UIInspector::send_cursor(bool clicked, Point<int16_t> cursor_position)
	{
		cursor_pos = cursor_position;

		if (clicked && InspectorMode::is_enabled())
		{
			UIElement* hovered = InspectorMode::get_hovered_element();
			if (hovered)
			{
				inspect_element(hovered, InspectorMode::get_hovered_type());
				return Cursor::State::CLICKING;
			}
		}

		return Cursor::State::IDLE;
	}

	void UIInspector::send_key(int32_t keycode, bool pressed, bool escape)
	{
		// F12 toggles inspector mode
		if (pressed && keycode == 123)  // F12 = 123
		{
			InspectorMode::toggle();
			if (!InspectorMode::is_enabled())
			{
				clear_inspection();
			}
		}
	}

	UIElement::Type UIInspector::get_type() const
	{
		return UIElement::Type::NONE;  // Special element, not in normal list
	}

	void UIInspector::inspect_element(UIElement* element, UIElement::Type type)
	{
		inspected_element = element;
		inspected_type = type;
		inspecting = true;
		update_info_text();

		// Log to console for debugging
		LOG(LOG_DEBUG, "[UIInspector] Inspecting: " << get_type_name(type)
			<< " at cursor (" << cursor_pos.x() << ", " << cursor_pos.y() << ")");
	}

	void UIInspector::clear_inspection()
	{
		inspected_element = nullptr;
		inspected_type = UIElement::Type::NONE;
		inspecting = false;
		info_lines.clear();
	}

	bool UIInspector::is_inspecting() const
	{
		return inspecting && inspected_element != nullptr;
	}

	std::string UIInspector::get_type_name(UIElement::Type type)
	{
		switch (type)
		{
			case UIElement::Type::NONE: return "NONE";
			case UIElement::Type::START: return "UIStart";
			case UIElement::Type::LOGIN: return "UILogin";
			case UIElement::Type::TOS: return "UITermsOfService";
			case UIElement::Type::GENDER: return "UIGender";
			case UIElement::Type::WORLDSELECT: return "UIWorldSelect";
			case UIElement::Type::REGION: return "UIRegion";
			case UIElement::Type::CHARSELECT: return "UICharSelect";
			case UIElement::Type::LOGINWAIT: return "UILoginWait";
			case UIElement::Type::RACESELECT: return "UIRaceSelect";
			case UIElement::Type::CLASSCREATION: return "UIClassCreation";
			case UIElement::Type::SOFTKEYBOARD: return "UISoftKeyboard";
			case UIElement::Type::LOGINNOTICE: return "UILoginNotice";
			case UIElement::Type::LOGINNOTICE_CONFIRM: return "UILoginNoticeConfirm";
			case UIElement::Type::STATUSMESSENGER: return "UIStatusMessenger";
			case UIElement::Type::STATUSBAR: return "UIStatusBar";
			case UIElement::Type::CHATBAR: return "UIChatBar";
			case UIElement::Type::BUFFLIST: return "UIBuffList";
			case UIElement::Type::NOTICE: return "UINotice";
			case UIElement::Type::NPCTALK: return "UINpcTalk";
			case UIElement::Type::SHOP: return "UIShop";
			case UIElement::Type::STATSINFO: return "UIStatsInfo";
			case UIElement::Type::ITEMINVENTORY: return "UIItemInventory";
			case UIElement::Type::EQUIPINVENTORY: return "UIEquipInventory";
			case UIElement::Type::SKILLBOOK: return "UISkillBook";
			case UIElement::Type::QUESTLOG: return "UIQuestLog";
			case UIElement::Type::WORLDMAP: return "UIWorldMap";
			case UIElement::Type::USERLIST: return "UIUserList";
			case UIElement::Type::MINIMAP: return "UIMiniMap";
			case UIElement::Type::CHANNEL: return "UIChannel";
			case UIElement::Type::CHAT: return "UIChat";
			case UIElement::Type::CHATRANK: return "UIChatRank";
			case UIElement::Type::JOYPAD: return "UIJoypad";
			case UIElement::Type::EVENT: return "UIEvent";
			case UIElement::Type::KEYCONFIG: return "UIKeyConfig";
			case UIElement::Type::OPTIONMENU: return "UIOptionMenu";
			case UIElement::Type::QUIT: return "UIQuit";
			case UIElement::Type::CHARINFO: return "UICharInfo";
			case UIElement::Type::CASHSHOP: return "UICashShop";
			default: return "Unknown";
		}
	}
}
