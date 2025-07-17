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
#include "UINpcTalk.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Gameplay/Stage.h"

#include "../../Net/Packets/NpcInteractionPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UINpcTalk::UINpcTalk() : offset(0), unitrows(0), rowmax(0), show_slider(false), draw_text(false), formatted_text(""), formatted_text_pos(0), timestep(0)
	{
		// v92 uses UIWindow.img, newer versions use UIWindow2.img
		nl::node UtilDlgEx = nl::nx::UI["UIWindow.img"]["UtilDlgEx"];
		if (!UtilDlgEx) {
			UtilDlgEx = nl::nx::UI["UIWindow2.img"]["UtilDlgEx"];
		}
		
		if (!UtilDlgEx) {
			LOG(LOG_ERROR, "[UINpcTalk] UtilDlgEx not found in UIWindow.img or UIWindow2.img");
			return;
		}

		top = UtilDlgEx["t"];
		fill = UtilDlgEx["c"];
		bottom = UtilDlgEx["s"];
		nametag = UtilDlgEx["bar"];

		min_height = 8 * fill.height() + 14;

		// Some buttons might not exist in v92
		if (UtilDlgEx["BtAllLevel"]) buttons[Buttons::ALLLEVEL] = std::make_unique<MapleButton>(UtilDlgEx["BtAllLevel"]);
		if (UtilDlgEx["BtClose"]) buttons[Buttons::CLOSE] = std::make_unique<MapleButton>(UtilDlgEx["BtClose"]);
		if (UtilDlgEx["BtMyLevel"]) buttons[Buttons::MYLEVEL] = std::make_unique<MapleButton>(UtilDlgEx["BtMyLevel"]);
		if (UtilDlgEx["BtNext"]) buttons[Buttons::NEXT] = std::make_unique<MapleButton>(UtilDlgEx["BtNext"]);
		if (UtilDlgEx["BtNo"]) buttons[Buttons::NO] = std::make_unique<MapleButton>(UtilDlgEx["BtNo"]);
		if (UtilDlgEx["BtOK"]) buttons[Buttons::OK] = std::make_unique<MapleButton>(UtilDlgEx["BtOK"]);
		if (UtilDlgEx["BtPrev"]) buttons[Buttons::PREV] = std::make_unique<MapleButton>(UtilDlgEx["BtPrev"]);
		if (UtilDlgEx["BtQAfter"]) buttons[Buttons::QAFTER] = std::make_unique<MapleButton>(UtilDlgEx["BtQAfter"]);
		if (UtilDlgEx["BtQCNo"]) buttons[Buttons::QCNO] = std::make_unique<MapleButton>(UtilDlgEx["BtQCNo"]);
		if (UtilDlgEx["BtQCYes"]) buttons[Buttons::QCYES] = std::make_unique<MapleButton>(UtilDlgEx["BtQCYes"]);
		if (UtilDlgEx["BtQGiveup"]) buttons[Buttons::QGIVEUP] = std::make_unique<MapleButton>(UtilDlgEx["BtQGiveup"]);
		if (UtilDlgEx["BtQNo"]) buttons[Buttons::QNO] = std::make_unique<MapleButton>(UtilDlgEx["BtQNo"]);
		if (UtilDlgEx["BtQStart"]) buttons[Buttons::QSTART] = std::make_unique<MapleButton>(UtilDlgEx["BtQStart"]);
		if (UtilDlgEx["BtQYes"]) buttons[Buttons::QYES] = std::make_unique<MapleButton>(UtilDlgEx["BtQYes"]);
		if (UtilDlgEx["BtYes"]) buttons[Buttons::YES] = std::make_unique<MapleButton>(UtilDlgEx["BtYes"]);

		name = Text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::WHITE);

		onmoved = [&](bool upwards)
		{
			int16_t shift = upwards ? -unitrows : unitrows;
			bool above = offset + shift >= 0;
			bool below = offset + shift <= rowmax - unitrows;

			if (above && below)
				offset += shift;
		};

		UI::get().remove_textfield();
	}

	void UINpcTalk::draw(float inter) const
	{
		Point<int16_t> drawpos = position;
		
		// Draw dialog frame only if textures are valid
		if (top.is_valid()) {
			top.draw(drawpos);
			drawpos.shift_y(top.height());
		}
		
		if (fill.is_valid()) {
			fill.draw(DrawArgument(drawpos, Point<int16_t>(0, height)));
			drawpos.shift_y(height);
		}
		
		if (bottom.is_valid()) {
			bottom.draw(drawpos);
			drawpos.shift_y(bottom.height());
		}

		UIElement::draw(inter);

		// Draw speaker info only if components are valid
		if (top.is_valid() && bottom.is_valid()) {
			int16_t speaker_y = (top.height() + height + bottom.height()) / 2;
			Point<int16_t> speaker_pos = position + Point<int16_t>(22, 11 + speaker_y);
			Point<int16_t> center_pos = speaker_pos + Point<int16_t>(nametag.width() / 2, 0);

			if (speaker.is_valid()) {
				speaker.draw(DrawArgument(center_pos, true));
			}
			
			if (nametag.is_valid()) {
				nametag.draw(speaker_pos);
			}
			
			name.draw(center_pos + Point<int16_t>(0, -4));
		}

		if (show_slider)
		{
			int16_t text_min_height = position.y() + top.height() - 1;
			text.draw(position + Point<int16_t>(162, 19 - offset * 400), Range<int16_t>(text_min_height, text_min_height + height - 18));
			slider.draw(position);
		}
		else
		{
			int16_t y_adj = height - min_height;
			text.draw(position + Point<int16_t>(166, 48 - y_adj));
		}
	}

	void UINpcTalk::update()
	{
		UIElement::update();

		if (draw_text)
		{
			if (timestep > 4)
			{
				if (formatted_text_pos < formatted_text.size())
				{
					std::string t = text.get_text();
					char c = formatted_text[formatted_text_pos];

					text.change_text(t + c);

					formatted_text_pos++;
					timestep = 0;
				}
				else
				{
					draw_text = false;
				}
			}
			else
			{
				timestep++;
			}
		}
	}

	Button::State UINpcTalk::button_pressed(uint16_t buttonid)
	{
		deactivate();

		switch (type)
		{
			case TalkType::SENDNEXT:
			case TalkType::SENDOK:
			{
				// Type = 0
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::NEXT:
					case Buttons::OK:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDNEXTPREV:
			{
				// Type = 0
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::NEXT:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
					case Buttons::PREV:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDYESNO:
			{
				// Type = 1
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::NO:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::YES:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDACCEPTDECLINE:
			{
				// Type = 1
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, -1).dispatch();
						break;
					case Buttons::QNO:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::QYES:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDGETTEXT:
			{
				// TODO: What is this?
				break;
			}
			case TalkType::SENDGETNUMBER:
			{
				// Type = 3
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					case Buttons::OK:
						NpcTalkMorePacket(type, 1).dispatch();
						break;
				}

				break;
			}
			case TalkType::SENDSIMPLE:
			{
				// Type = 4
				switch (buttonid)
				{
					case Buttons::CLOSE:
						NpcTalkMorePacket(type, 0).dispatch();
						break;
					default:
						NpcTalkMorePacket(0).dispatch(); // TODO: Selection
						break;
				}

				break;
			}
			default:
			{
				break;
			}
		}

		return Button::State::NORMAL;
	}

	Cursor::State UINpcTalk::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Point<int16_t> cursor_relative = cursorpos - position;

		if (show_slider && slider.isenabled())
			if (Cursor::State sstate = slider.send_cursor(cursor_relative, clicked))
				return sstate;

		Cursor::State estate = UIElement::send_cursor(clicked, cursorpos);

		if (estate == Cursor::State::CLICKING && clicked && draw_text)
		{
			draw_text = false;
			text.change_text(formatted_text);
		}

		return estate;
	}

	void UINpcTalk::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed && escape)
		{
			deactivate();

			NpcTalkMorePacket(type, 0).dispatch();
		}
	}

	UIElement::Type UINpcTalk::get_type() const
	{
		return TYPE;
	}

	UINpcTalk::TalkType UINpcTalk::get_by_value(int8_t value)
	{
		if (value > TalkType::NONE && value < TalkType::LENGTH)
			return static_cast<TalkType>(value);

		return TalkType::NONE;
	}

	// TODO: Move this to GraphicsGL?
	std::string UINpcTalk::format_text(const std::string& tx, const int32_t& npcid)
	{
		std::string formatted_text = tx;
		size_t begin = formatted_text.find("#p");

		if (begin != std::string::npos)
		{
			size_t end = formatted_text.find("#", begin + 1);

			if (end != std::string::npos)
			{
				std::string namestr = nl::nx::String["Npc.img"][std::to_string(npcid)]["name"];
				formatted_text.replace(begin, end - begin, namestr);
			}
		}

		begin = formatted_text.find("#h");

		if (begin != std::string::npos)
		{
			size_t end = formatted_text.find("#", begin + 1);

			if (end != std::string::npos)
			{
				std::string charstr = Stage::get().get_player().get_name();
				formatted_text.replace(begin, end - begin, charstr);
			}
		}

		begin = formatted_text.find("#t");

		if (begin != std::string::npos)
		{
			size_t end = formatted_text.find("#", begin + 1);

			if (end != std::string::npos)
			{
				size_t b = begin + 2;
				int32_t itemid = std::stoi(formatted_text.substr(b, end - b));
				std::string itemname = nl::nx::String["Consume.img"][itemid]["name"];

				formatted_text.replace(begin, end - begin, itemname);
			}
		}

		return formatted_text;
	}

	void UINpcTalk::change_text(int32_t npcid, int8_t msgtype, int16_t, int8_t speakerbyte, const std::string& tx)
	{
		type = get_by_value(msgtype);

		timestep = 0;
		draw_text = true;
		formatted_text_pos = 0;
		formatted_text = format_text(tx, npcid);

		text = Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::DARKGREY, formatted_text, 320);

		int16_t text_height = text.height();

		text.change_text("");

		if (speakerbyte == 0)
		{
			std::string strid = std::to_string(npcid);
			strid.insert(0, 7 - strid.size(), '0');
			strid.append(".img");

			speaker = nl::nx::Npc[strid]["stand"]["0"];

			std::string namestr = nl::nx::String["Npc.img"][std::to_string(npcid)]["name"];
			name.change_text(namestr);
		}
		else
		{
			speaker = Texture();
			name.change_text("");
		}

		height = min_height;
		show_slider = false;

		if (text_height > height)
		{
			if (text_height > MAX_HEIGHT)
			{
				height = MAX_HEIGHT;
				show_slider = true;
				rowmax = text_height / 400 + 1;
				unitrows = 1;

				int16_t slider_y = top.height() - 7;
				// Use simpler slider for v92
				slider = Slider(Slider::Type::LINE_CYAN, Range<int16_t>(slider_y, slider_y + height - 20), top.width() - 26, unitrows, rowmax, onmoved);
			}
			else
			{
				height = text_height;
			}
		}

		for (auto& button : buttons)
		{
			button.second->set_active(false);
			button.second->set_state(Button::State::NORMAL);
		}

		int16_t y_cord = height + 48;

		buttons[Buttons::CLOSE]->set_position(Point<int16_t>(9, y_cord));
		buttons[Buttons::CLOSE]->set_active(true);

		switch (type)
		{
			case TalkType::SENDOK:
			{
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDYESNO:
			{
				Point<int16_t> yes_position = Point<int16_t>(389, y_cord);

				buttons[Buttons::YES]->set_position(yes_position);
				buttons[Buttons::YES]->set_active(true);

				buttons[Buttons::NO]->set_position(yes_position + Point<int16_t>(65, 0));
				buttons[Buttons::NO]->set_active(true);
				break;
			}
			case TalkType::SENDNEXT:
			{
				buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::NEXT]->set_active(true);
				break;
			}
			case TalkType::SENDNEXTPREV:
			{
				buttons[Buttons::NEXT]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::NEXT]->set_active(true);
				
				buttons[Buttons::PREV]->set_position(Point<int16_t>(389, y_cord));
				buttons[Buttons::PREV]->set_active(true);
				break;
			}
			case TalkType::SENDACCEPTDECLINE:
			{
				Point<int16_t> yes_position = Point<int16_t>(389, y_cord);

				buttons[Buttons::QYES]->set_position(yes_position);
				buttons[Buttons::QYES]->set_active(true);

				buttons[Buttons::QNO]->set_position(yes_position + Point<int16_t>(65, 0));
				buttons[Buttons::QNO]->set_active(true);
				break;
			}
			case TalkType::SENDGETTEXT:
			{
				// TODO: Implement text input dialog
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDGETNUMBER:
			{
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			case TalkType::SENDSIMPLE:
			{
				buttons[Buttons::OK]->set_position(Point<int16_t>(471, y_cord));
				buttons[Buttons::OK]->set_active(true);
				break;
			}
			default:
			{
				break;
			}
		}

		position = Point<int16_t>(400 - top.width() / 2, 240 - height / 2);
		dimension = Point<int16_t>(top.width(), height + 120);
	}
}