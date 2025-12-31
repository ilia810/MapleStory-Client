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
#include "UIChatBar.h"

#include "../UI.h"

#include "../Components/MapleButton.h"

#include "../../Graphics/Geometry.h"
#include "../../Graphics/DrawArgument.h"
#include "../../Net/Packets/MessagingPackets.h"

#include <iostream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIChatBar::UIChatBar() : temp_view_x(0), temp_view_y(0), drag_direction(DragDirection::NONE), view_input(false), view_adjusted(false), position_adjusted(false)
	{
		// V87 compatibility: Check which StatusBar version exists
		bool is_v87 = nl::nx::UI["StatusBar3.img"].name().empty();
		
		nl::node input, view;
		
		if (is_v87) {
			// V87: Chat assets are minimal - most UI elements might not exist
			// We'll need to create a simplified chat bar
			nl::node statusBar = nl::nx::UI["StatusBar.img"];
			
			// V87 doesn't have the complex chat structure
			// Create empty nodes to prevent crashes
			input = nl::node();
			view = nl::node();
		} else {
			// Modern versions: StatusBar3.img with chat/ingame structure
			nl::node statusBar3 = nl::nx::UI["StatusBar3.img"];
			nl::node chat = statusBar3["chat"];
			nl::node ingame = chat["ingame"];
			
			input = ingame["input"];
			view = ingame["view"];
		}
		if (!is_v87) {
			// Modern versions have the full chat UI structure
			nl::node max = view["max"];
			nl::node min = view["min"];

			drag = view["drag"];

			max_textures.emplace_back(max["top"]);
			max_textures.emplace_back(max["center"]);
			max_textures.emplace_back(max["bottom"]);

			min_textures.emplace_back(min["top"]);
			min_textures.emplace_back(min["center"]);
			min_textures.emplace_back(min["bottom"]);
		} else {
			// V87: Create placeholder textures to prevent crashes
			// The chat UI will be very basic
			drag = Texture();
			
			// Add empty textures
			max_textures.emplace_back(Texture());
			max_textures.emplace_back(Texture());
			max_textures.emplace_back(Texture());
			
			min_textures.emplace_back(Texture());
			min_textures.emplace_back(Texture());
			min_textures.emplace_back(Texture());
		}

		if (!is_v87) {
			min_x = min_textures[0].width();
			max_x = max_textures[0].width();
			top_y = min_textures[0].height();
			center_y = min_textures[1].height();
			bottom_y = min_textures[2].height();
			btMin_x = Texture(view["btMin"]["normal"]["0"]).width();

			input_textures.emplace_back(input["layer:backgrnd"]);
			input_textures.emplace_back(input["layer:chatEnter"]);
		} else {
			// V87: Use default dimensions
			min_x = 200;  // Default width
			max_x = 400;  // Default expanded width
			top_y = 20;   // Default heights
			center_y = 60;
			bottom_y = 20;
			btMin_x = 16;
			
			// Add empty textures
			input_textures.emplace_back(Texture());
			input_textures.emplace_back(Texture());
		}

		if (!is_v87) {
			input_bg_x = input_textures[0].width();
			input_bg_y = input_textures[0].height();
			input_max_x = input_textures[1].width();

			Point<int16_t> input_origin = input_textures[1].get_origin().abs();
			input_origin_x = input_origin.x();
			input_origin_y = input_origin.y();
		} else {
			// V87: Use default dimensions
			input_bg_x = 500;
			input_bg_y = 60;
			input_max_x = 500;
			input_origin_x = 0;
			input_origin_y = 0;
		}

		min_view_y = Constants::Constants::get().get_viewheight() - input_bg_y;
		user_view_x = Setting<ChatViewX>::get().load();

		if (user_view_x == 0)
			user_view_x = min_x;

		user_view_y = Setting<ChatViewY>::get().load();

		if (user_view_y == 0)
		{
			// 20 pixels for the extra height by default
			user_view_y = top_y + center_y + bottom_y + 20;
		}

		if (!is_v87) {
			int16_t btMax_x = Texture(view["btMax"]["normal"]["0"]).width();

			// Five pixels left and seven pixels up for padding
			buttons[Buttons::BtMax] = std::make_unique<MapleButton>(view["btMax"], Point<int16_t>(min_x - btMax_x - 5, -7));
			buttons[Buttons::BtMin] = std::make_unique<MapleButton>(view["btMin"]);

			input_btns_pos = Point<int16_t>(input_max_x - (input_bg_x - user_view_x) + input_origin_x - 17, 15 + input_origin_y + 1);
			int16_t input_btns_padding = 3;
			input_btns_x = Texture(input["button:chat"]["normal"]["0"]).width() + input_btns_padding;

			// Create basic chat button
			buttons[Buttons::BtChat] = std::make_unique<MapleButton>(input["button:chat"], input_btns_pos + Point<int16_t>(input_btns_x * 0, 0));
		} else {
			// V87: No chat UI buttons exist
			input_btns_pos = Point<int16_t>(0, 0);
			input_btns_x = 20; // Default button width
		}
		
		// v87 compatibility: only create buttons if they exist
		int16_t button_index = 1;
		
		if (input["button:itemLink"])
		{
			buttons[Buttons::BtItemLink] = std::make_unique<MapleButton>(input["button:itemLink"], input_btns_pos + Point<int16_t>(input_btns_x * button_index, 0));
			buttons[Buttons::BtItemLink]->set_active(false);
			button_index++;
		}
		
		if (input["button:chatEmoticon"])
		{
			buttons[Buttons::BtChatEmoticon] = std::make_unique<MapleButton>(input["button:chatEmoticon"], input_btns_pos + Point<int16_t>(input_btns_x * button_index, 0));
			buttons[Buttons::BtChatEmoticon]->set_active(false);
			button_index++;
		}
		
		if (input["button:help"])
		{
			buttons[Buttons::BtHelp] = std::make_unique<MapleButton>(input["button:help"], input_btns_pos + Point<int16_t>(input_btns_x * button_index, 0));
			buttons[Buttons::BtHelp]->set_active(false);
			button_index++;
		}
		
		if (input["button:outChat"])
		{
			buttons[Buttons::BtOutChat] = std::make_unique<MapleButton>(input["button:outChat"], input_btns_pos + Point<int16_t>(input_btns_x * button_index, 0));
			buttons[Buttons::BtOutChat]->set_active(false);
		}

		if (buttons[Buttons::BtChat]) {
			buttons[Buttons::BtChat]->set_active(false);
		}

		// Initialize chat target selector early (before input_text) since get_input_text_position() depends on it
		nl::node channel_node = nl::nx::UI["UIWindow.img"]["Channel"];
		if (channel_node["channel2"])
			channel_texture = Texture(channel_node["channel2"]);

		current_chat_target = MessageGroup::ALL;
		channel_text = Text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, get_chat_target_text());

		int16_t input_text_limit = 70;
		int16_t input_text_marker_height = 11;

		input_text = Textfield(
			Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK,  // Black text in typing area
			Rectangle<int16_t>(get_input_text_position(), get_input_text_position() + Point<int16_t>(283, INPUT_TEXT_HEIGHT)),
			input_text_limit, input_text_marker_height
		);

		input_text.set_enter_callback(
			[&](std::string message)
			{
				input_text_enter_callback(message);
			}
		);

		input_text.set_key_callback(
			KeyAction::Id::ESCAPE,
			[&]()
			{
				input_text_escape_callback();
			}
		);

		input_text.set_key_callback(
			KeyAction::Id::UP,
			[&]()
			{
				change_message(true);
			}
		);

		input_text.set_key_callback(
			KeyAction::Id::DOWN,
			[&]()
			{
				change_message(false);
			}
		);

		dragarea = drag.get_dimensions();

		// Initialize scroll offset
		chat_scroll_offset = 0;

		// Load VScr4 textures for scroll arrows and scrollbar
		nl::node vscr4 = nl::nx::UI["Basic.img"]["VScr4"];
		nl::node vscr4_enabled = vscr4["enabled"];
		nl::node vscr4_disabled = vscr4["disabled"];

		// Load scrollbar textures (base from disabled, thumb from enabled)
		scrollbar_base = vscr4_disabled["base"];
		scrollbar_thumb = vscr4_enabled["thumb0"];

		// Load arrow textures (0 = normal state)
		arrow_up_tex = vscr4_enabled["prev0"];
		arrow_down_tex = vscr4_enabled["next0"];
		int16_t arrow_width = arrow_up_tex.width();
		int16_t arrow_height = arrow_up_tex.height();

		// Button positions for collapsed view (565x26 box):
		// - Scroll arrows at right edge, inside the box, up on top, down below
		// - Expand button 5px to the left of arrows
		// Arrows will be drawn manually, not as MapleButtons

		// Expand button 5px to the left of arrows
		nl::node softkey_btns = nl::nx::UI["UIWindow.img"]["SoftKeyboard"]["Bt"]["0"];
		Texture expand_tex = softkey_btns["BtMax"]["normal"]["0"];
		int16_t expand_width = expand_tex.width();
		int16_t arrow_x = CHAT_WIDTH - arrow_width;
		int16_t expand_x = arrow_x - expand_width - CHAT_PADDING;
		int16_t expand_y = -CHAT_COLLAPSED_HEIGHT + (CHAT_COLLAPSED_HEIGHT - expand_tex.height()) / 2;

		if (softkey_btns["BtMax"]) {
			buttons[Buttons::BtExpand] = std::make_unique<MapleButton>(softkey_btns["BtMax"], Point<int16_t>(expand_x, expand_y));
		}
		if (softkey_btns["BtMin"]) {
			// Collapse button at same position as expand
			buttons[Buttons::BtCollapse] = std::make_unique<MapleButton>(softkey_btns["BtMin"], Point<int16_t>(expand_x, expand_y));
		}

		toggle_view(false, false);  // Always start collapsed

		// Position chat relative to centered statusbar
		int16_t VWIDTH = Constants::Constants::get().get_viewwidth();
		int16_t VHEIGHT = Constants::Constants::get().get_viewheight();
		int16_t statusbar_height = (VWIDTH <= 1024) ? 75 : 80;
		int16_t statusbar_y = VHEIGHT - statusbar_height + 9;

		// Calculate centered status bar position (same as UIStatusBar)
		nl::node statusBar = nl::nx::UI["StatusBar.img"];
		nl::node base = statusBar["base"];
		int16_t base_width = 0;
		if (base["backgrnd"]) {
			Texture base_tex(base["backgrnd"]);
			base_width = base_tex.width();
		}
		int16_t statusbar_x = (VWIDTH - base_width) / 2;

		// Position is the BOTTOM-LEFT of the chat box
		// Chat position relative to centered statusbar: (statusbar_x + 4, statusbar_y + 30)
		position = Point<int16_t>(statusbar_x + 4, statusbar_y + 30);

#if LOG_LEVEL >= LOG_UI
		dragarea_box = ColorBox(dragarea.x(), dragarea.y(), Color::Name::BLUE, 0.5f);
		input_box = ColorBox(input_bg_x, input_bg_y, Color::Name::RED, 0.5f);
#endif

		show_message("[Welcome] Welcome to MapleStory!!", MessageType::YELLOW);
	}

	void UIChatBar::draw(float inter) const
	{
		// Chat box top-left position (position is bottom-left of collapsed chat)
		Point<int16_t> box_top_left = position - Point<int16_t>(0, CHAT_COLLAPSED_HEIGHT);

		if (view_max)
		{
			// Expanded view: background + messages + scrollbar
			// The expanded background sits ABOVE the collapsed chat area
			// Its bottom border touches the top border of the collapsed area
			Point<int16_t> expanded_top_left = box_top_left - Point<int16_t>(0, CHAT_EXPANDED_HEIGHT);

			// Draw semi-transparent black background for expanded view only
			ColorBox background(CHAT_WIDTH, CHAT_EXPANDED_HEIGHT, Color::Name::BLACK, 0.7f);
			background.draw(expanded_top_left);

			// Draw messages
			int16_t message_line_height = 13;
			int16_t text_area_height = CHAT_EXPANDED_HEIGHT - 10;
			int16_t max_visible_lines = text_area_height / message_line_height;

			int16_t message_y = CHAT_EXPANDED_HEIGHT - CHAT_PADDING - message_line_height;

			int start_index = static_cast<int>(message_history.size()) - 1 - chat_scroll_offset;
			int lines_drawn = 0;

			for (int i = start_index; i >= 0 && lines_drawn < max_visible_lines; i--)
			{
				Point<int16_t> msg_pos = expanded_top_left + Point<int16_t>(CHAT_PADDING, message_y);
				message_history[i].text.draw(msg_pos);
				message_y -= message_line_height;
				lines_drawn++;
			}

			// Draw scrollbar on right side
			int16_t scrollbar_x = CHAT_WIDTH - 15;
			int16_t scrollbar_y = CHAT_PADDING;

			// Draw scrollbar base (stretched to fill height)
			scrollbar_base.draw(expanded_top_left + Point<int16_t>(scrollbar_x, scrollbar_y));

			// Draw scroll arrows at top and bottom of scrollbar area
			arrow_up_tex.draw(expanded_top_left + Point<int16_t>(scrollbar_x, scrollbar_y));
			arrow_down_tex.draw(expanded_top_left + Point<int16_t>(scrollbar_x, CHAT_EXPANDED_HEIGHT - CHAT_PADDING - arrow_down_tex.height()));

			// Draw scrollbar thumb
			if (message_history.size() > 0)
			{
				int16_t scrollbar_height = CHAT_EXPANDED_HEIGHT - 2 * CHAT_PADDING - arrow_up_tex.height() - arrow_down_tex.height();
				float scroll_ratio = static_cast<float>(chat_scroll_offset) / static_cast<float>(message_history.size());
				int16_t thumb_y = scrollbar_y + arrow_up_tex.height() + static_cast<int16_t>(scroll_ratio * (scrollbar_height - scrollbar_thumb.height()));
				scrollbar_thumb.draw(expanded_top_left + Point<int16_t>(scrollbar_x, thumb_y));
			}
		}
		else
		{
			// Collapsed view: gray background (#757676) with message text and arrows
			ColorBox collapsed_bg(CHAT_WIDTH, CHAT_COLLAPSED_HEIGHT, Color::Name::CHATGRAY, 1.0f);
			collapsed_bg.draw(box_top_left);

			const size_t size = message_history.size();

			if (size > 0)
			{
				int msg_index = static_cast<int>(size) - 1 - chat_scroll_offset;
				if (msg_index >= 0 && msg_index < static_cast<int>(size))
				{
					// Center text vertically in the 26px height box
					int16_t text_y = (CHAT_COLLAPSED_HEIGHT - 13) / 2 - 3;
					Point<int16_t> text_pos = box_top_left + Point<int16_t>(CHAT_PADDING, text_y);
					message_history[msg_index].text.draw(text_pos);
				}
			}

			// Draw scroll arrows at right edge (up arrow on top, down arrow below)
			int16_t arrow_width = arrow_up_tex.width();
			int16_t arrow_height = arrow_up_tex.height();
			int16_t arrow_x = CHAT_WIDTH - arrow_width;
			int16_t arrow_up_y = (CHAT_COLLAPSED_HEIGHT - arrow_height * 2) / 2;
			int16_t arrow_down_y = arrow_up_y + arrow_height;

			arrow_up_tex.draw(box_top_left + Point<int16_t>(arrow_x, arrow_up_y));
			arrow_down_tex.draw(box_top_left + Point<int16_t>(arrow_x, arrow_down_y));
		}

		// Draw input area at the collapsed chat position when typing is active
		if (view_input)
		{
			// Draw white background for the input area
			ColorBox input_bg(CHAT_WIDTH, CHAT_COLLAPSED_HEIGHT, Color::Name::WHITE, 1.0f);
			input_bg.draw(box_top_left);

			// Draw channel selector texture at left, vertically centered
			int16_t channel_tex_height = channel_texture.height();
			int16_t channel_tex_width = channel_texture.width();
			int16_t channel_y = (CHAT_COLLAPSED_HEIGHT - channel_tex_height) / 2;
			Point<int16_t> channel_pos = box_top_left + Point<int16_t>(CHAT_PADDING, channel_y);
			channel_texture.draw(channel_pos);

			// Draw channel text centered inside the texture
			int16_t text_x = CHAT_PADDING + (channel_tex_width - channel_text.width()) / 2;
			int16_t text_y = (CHAT_COLLAPSED_HEIGHT - 13) / 2 - 3;
			channel_text.draw(box_top_left + Point<int16_t>(text_x, text_y));

			// Draw the text cursor/input (bounds are already set correctly in update())
			// Cursor adjusted by (1, -3) relative to text
			input_text.draw(Point<int16_t>(0, 0), Point<int16_t>(1, -3));

			// Draw disabled scroll arrows at right edge
			int16_t arrow_width = arrow_up_tex.width();
			int16_t arrow_height = arrow_up_tex.height();
			int16_t arrow_x = CHAT_WIDTH - arrow_width;
			int16_t arrow_up_y = (CHAT_COLLAPSED_HEIGHT - arrow_height * 2) / 2;
			int16_t arrow_down_y = arrow_up_y + arrow_height;

			// Draw with reduced opacity (0.3) to indicate disabled state
			arrow_up_tex.draw(DrawArgument(box_top_left + Point<int16_t>(arrow_x, arrow_up_y), 0.3f));
			arrow_down_tex.draw(DrawArgument(box_top_left + Point<int16_t>(arrow_x, arrow_down_y), 0.3f));
		}

		// Draw channel selector in expanded view as well (at the collapsed chat position)
		if (view_max && !view_input)
		{
			int16_t channel_tex_height = channel_texture.height();
			int16_t channel_tex_width = channel_texture.width();
			int16_t channel_y = (CHAT_COLLAPSED_HEIGHT - channel_tex_height) / 2;
			Point<int16_t> channel_pos = box_top_left + Point<int16_t>(CHAT_PADDING, channel_y);
			channel_texture.draw(channel_pos);

			// Draw channel text centered inside the texture
			int16_t text_x = CHAT_PADDING + (channel_tex_width - channel_text.width()) / 2;
			int16_t text_y = (CHAT_COLLAPSED_HEIGHT - 13) / 2 - 3;
			channel_text.draw(box_top_left + Point<int16_t>(text_x, text_y));
		}

		UIElement::draw(inter);

#if LOG_LEVEL >= LOG_UI
		dimension_box.draw(get_position());
		dragarea_box.draw(get_dragarea_position());

		if (view_max)
		{
			top_box.draw(get_position());
			bottom_box.draw(get_position() + Point<int16_t>(0, dimension.y() - 3));
			left_box.draw(get_position());
			right_box.draw(get_position() + Point<int16_t>(dimension.x() - 3, 0));
		}
#endif
	}

	void UIChatBar::update()
	{
		// Update text field to span most of the collapsed chat width (minus channel selector, arrows and padding)
		int16_t channel_tex_width = channel_texture.width();
		int16_t text_width = CHAT_WIDTH - CHAT_PADDING * 3 - channel_tex_width - 50;  // Leave room for channel selector, arrows and expand button

		// Input text starts after the channel selector
		// Adjusted by (0, -5) for vertical alignment
		Point<int16_t> box_top_left = position - Point<int16_t>(0, CHAT_COLLAPSED_HEIGHT);
		int16_t input_offset_x = CHAT_PADDING + channel_tex_width + CHAT_PADDING;
		Point<int16_t> input_pos = box_top_left + Point<int16_t>(input_offset_x, (CHAT_COLLAPSED_HEIGHT - 11) / 2 - 5);

		input_text.update(input_pos, Point<int16_t>(text_width, INPUT_TEXT_HEIGHT));
	}

	Button::State UIChatBar::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
			case ms::UIChatBar::BtMax:
			{
				toggle_view(true, true);

				return Button::State::NORMAL;
			}
			case ms::UIChatBar::BtMin:
			{
				toggle_view(false, true);

				return Button::State::NORMAL;
			}
			case ms::UIChatBar::BtExpand:
			{
				// Expand the chat
				toggle_view(true, true);

				return Button::State::NORMAL;
			}
			case ms::UIChatBar::BtCollapse:
			{
				// Collapse the chat
				toggle_view(false, true);

				return Button::State::NORMAL;
			}
			default:
			{
				return Button::State::DISABLED;
			}
		}
	}

	bool UIChatBar::is_in_range(Point<int16_t> cursor_position) const
	{
		if (temp_view_y == 0 && temp_view_x == 0)
		{
			// Collapsed area bounds
			Point<int16_t> collapsed_top_left = position - Point<int16_t>(0, CHAT_COLLAPSED_HEIGHT);
			Rectangle<int16_t> collapsed_bounds = Rectangle<int16_t>(collapsed_top_left, collapsed_top_left + Point<int16_t>(CHAT_WIDTH, CHAT_COLLAPSED_HEIGHT));

			if (view_max)
			{
				// Expanded area is ABOVE the collapsed area
				Point<int16_t> expanded_top_left = collapsed_top_left - Point<int16_t>(0, CHAT_EXPANDED_HEIGHT);
				Rectangle<int16_t> expanded_bounds = Rectangle<int16_t>(expanded_top_left, expanded_top_left + Point<int16_t>(CHAT_WIDTH, CHAT_EXPANDED_HEIGHT));

				return collapsed_bounds.contains(cursor_position) || expanded_bounds.contains(cursor_position);
			}

			return collapsed_bounds.contains(cursor_position);
		}
		else
		{
			Rectangle<int16_t> bounds = Rectangle<int16_t>(
				Point<int16_t>(0, 0),
				Point<int16_t>(
					Constants::Constants::get().get_viewwidth(),
					Constants::Constants::get().get_viewheight()
				)
			);

			return bounds.contains(cursor_position);
		}
	}

	Cursor::State UIChatBar::send_cursor(bool clicked, Point<int16_t> cursor_position)
	{
		if (view_input && temp_view_y == 0 && temp_view_x == 0)
			if (Cursor::State new_state = input_text.send_cursor(cursor_position, clicked))
				return new_state;

		// Handle channel selector clicks (when typing or expanded)
		if (clicked && (view_input || view_max))
		{
			Point<int16_t> box_top_left = position - Point<int16_t>(0, CHAT_COLLAPSED_HEIGHT);
			int16_t channel_tex_height = channel_texture.height();
			int16_t channel_tex_width = channel_texture.width();
			int16_t channel_y = (CHAT_COLLAPSED_HEIGHT - channel_tex_height) / 2;
			Point<int16_t> channel_pos = box_top_left + Point<int16_t>(CHAT_PADDING, channel_y);

			Rectangle<int16_t> channel_bounds(
				channel_pos,
				channel_pos + Point<int16_t>(channel_tex_width, channel_tex_height)
			);

			if (channel_bounds.contains(cursor_position))
			{
				cycle_chat_target();
				return Cursor::State::CLICKING;
			}
		}

		// Handle scroll arrow clicks in collapsed view
		if (clicked && !view_max)
		{
			int16_t current_height = CHAT_COLLAPSED_HEIGHT;
			Point<int16_t> box_top_left = position - Point<int16_t>(0, current_height);

			int16_t arrow_width = arrow_up_tex.width();
			int16_t arrow_height = arrow_up_tex.height();
			int16_t arrow_x = CHAT_WIDTH - arrow_width;
			int16_t arrow_up_y = (CHAT_COLLAPSED_HEIGHT - arrow_height * 2) / 2;
			int16_t arrow_down_y = arrow_up_y + arrow_height;

			// Check up arrow
			Rectangle<int16_t> up_bounds(
				box_top_left + Point<int16_t>(arrow_x, arrow_up_y),
				box_top_left + Point<int16_t>(arrow_x + arrow_width, arrow_up_y + arrow_height)
			);
			if (up_bounds.contains(cursor_position))
			{
				scroll_chat(true);
				return Cursor::State::CLICKING;
			}

			// Check down arrow
			Rectangle<int16_t> down_bounds(
				box_top_left + Point<int16_t>(arrow_x, arrow_down_y),
				box_top_left + Point<int16_t>(arrow_x + arrow_width, arrow_down_y + arrow_height)
			);
			if (down_bounds.contains(cursor_position))
			{
				scroll_chat(false);
				return Cursor::State::CLICKING;
			}
		}

		if (clicked)
		{
			if (dragged)
			{
				if (temp_view_y == 0 && temp_view_x == 0)
				{
					Point<int16_t> new_pos = cursor_position - cursoroffset;
					int16_t new_pos_x = new_pos.x();
					int16_t new_pos_y = new_pos.y();

					if (new_pos_x < 0)
						new_pos.set_x(0);

					int16_t min_y = MIN_HEIGHT - 2 + drag.get_origin().y() * -1;

					if (view_input)
						min_y += user_view_y;

					if (new_pos_y < min_y)
						new_pos.set_y(min_y);

					int16_t max_x = Constants::Constants::get().get_viewwidth() - user_view_x;

					if (new_pos_x > max_x)
						new_pos.set_x(max_x);

					int16_t max_y = min_view_y;

					if (view_input)
						max_y -= input_bg_y;

					if (new_pos_y > max_y)
						new_pos.set_y(max_y);

					position = new_pos;

					return Cursor::State::CHATBARMOVE;
				}
				else
				{
					if (temp_view_x == 0)
					{
						if (drag_direction == DragDirection::DOWN)
						{
							// TODO: The top gets shifted by a pixel
							Point<int16_t> pos_y = cursor_position - position + Point<int16_t>(0, user_view_y) - Point<int16_t>(0, 13);
							Point<int16_t> pos = cursor_position - cursoroffset;

							if (pos_y.y() <= MIN_HEIGHT)
							{
								temp_view_y = MIN_HEIGHT;
								temp_position = position - Point<int16_t>(0, user_view_y) + Point<int16_t>(0, 13);
							}
							else if (pos_y.y() >= MAX_HEIGHT)
							{
								temp_view_y = MAX_HEIGHT;
								temp_position = position - Point<int16_t>(0, user_view_y) + Point<int16_t>(0, MAX_HEIGHT);
							}
							else
							{
								temp_view_y = pos_y.y();
								temp_position = Point<int16_t>(position.x(), pos.y());
							}
						}
						else
						{
							Point<int16_t> pos = position - cursor_position - Point<int16_t>(0, 13);

							if (pos.y() <= MIN_HEIGHT)
								temp_view_y = MIN_HEIGHT;
							else if (pos.y() >= MAX_HEIGHT)
								temp_view_y = MAX_HEIGHT;
							else
								temp_view_y = pos.y();
						}

#if LOG_LEVEL >= LOG_UI
						dimension = Point<int16_t>(user_view_x, top_y + center_y + bottom_y) + Point<int16_t>(0, temp_view_y);

						dimension_box = ColorBox(dimension.x(), dimension.y(), Color::Name::RED, 0.5f);
						left_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
						right_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
#endif

						return Cursor::State::CHATBARVDRAG;
					}
					else if (temp_view_y == 0)
					{
						if (drag_direction == DragDirection::LEFT)
						{
							// TODO: The right gets shifted by a pixel
							Point<int16_t> pos_x = position - cursor_position + Point<int16_t>(user_view_x, 0);
							Point<int16_t> pos = cursor_position - cursoroffset;

							if (pos_x.x() <= min_x)
							{
								temp_view_x = min_x;
								temp_position = position + Point<int16_t>(user_view_x, 0) - Point<int16_t>(min_x, 0);
							}
							else if (pos_x.x() >= max_x)
							{
								temp_view_x = max_x;
								temp_position = position - Point<int16_t>(max_x, 0) + Point<int16_t>(user_view_x, 0);
							}
							else
							{
								temp_view_x = pos_x.x();
								temp_position = Point<int16_t>(pos.x(), position.y());
							}
						}
						else
						{
							Point<int16_t> pos = cursor_position - position;

							if (pos.x() <= min_x)
								temp_view_x = min_x;
							else if (pos.x() >= max_x)
								temp_view_x = max_x;
							else
								temp_view_x = pos.x();
						}

#if LOG_LEVEL >= LOG_UI
						dimension = Point<int16_t>(temp_view_x, top_y + center_y + bottom_y) + Point<int16_t>(0, user_view_y);

						dimension_box = ColorBox(dimension.x(), dimension.y(), Color::Name::RED, 0.5f);
						top_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
						bottom_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
#endif

						return Cursor::State::CHATBARHDRAG;
					}
					else
					{
						if (drag_direction == DragDirection::DOWNLEFT)
						{
							int16_t temp_position_x, temp_position_y;

							Point<int16_t> pos = cursor_position - cursoroffset;

							// TODO: The right gets shifted by a pixel
							Point<int16_t> pos_x = position - cursor_position + Point<int16_t>(user_view_x, 0);

							if (pos_x.x() <= min_x)
							{
								temp_view_x = min_x;
								temp_position_x = position.x() + user_view_x - min_x;
							}
							else if (pos_x.x() >= max_x)
							{
								temp_view_x = max_x;
								temp_position_x = position.x() - max_x + user_view_x;
							}
							else
							{
								temp_view_x = pos_x.x();
								temp_position_x = pos.x();
							}

							// TODO: The top gets shifted by a pixel
							Point<int16_t> pos_y = cursor_position - position + Point<int16_t>(0, user_view_y) - Point<int16_t>(0, 13);

							if (pos_y.y() <= MIN_HEIGHT)
							{
								temp_view_y = MIN_HEIGHT;
								temp_position_y = position.y() - user_view_y + 13;
							}
							else if (pos_y.y() >= MAX_HEIGHT)
							{
								temp_view_y = MAX_HEIGHT;
								temp_position_y = position.y() - user_view_y + MAX_HEIGHT;
							}
							else
							{
								temp_view_y = pos_y.y();
								temp_position_y = pos.y();
							}

							temp_position = Point<int16_t>(temp_position_x, temp_position_y);
						}
						else
						{
							if (drag_direction == DragDirection::LEFT)
							{
								// TODO: The right gets shifted by a pixel
								Point<int16_t> pos_x = position - cursor_position + Point<int16_t>(user_view_x, 0);
								Point<int16_t> pos = cursor_position - cursoroffset;

								if (pos_x.x() <= min_x)
								{
									temp_view_x = min_x;
									temp_position = position + Point<int16_t>(user_view_x, 0) - Point<int16_t>(min_x, 0);
								}
								else if (pos_x.x() >= max_x)
								{
									temp_view_x = max_x;
									temp_position = position - Point<int16_t>(max_x, 0) + Point<int16_t>(user_view_x, 0);
								}
								else
								{
									temp_view_x = pos_x.x();
									temp_position = Point<int16_t>(pos.x(), position.y());
								}
							}
							else
							{
								Point<int16_t> pos_x = cursor_position - position;

								if (pos_x.x() <= min_x)
									temp_view_x = min_x;
								else if (pos_x.x() >= max_x)
									temp_view_x = max_x;
								else
									temp_view_x = pos_x.x();
							}

							if (drag_direction == DragDirection::DOWN)
							{
								// TODO: The top gets shifted by a pixel
								Point<int16_t> pos_y = cursor_position - position + Point<int16_t>(0, user_view_y) - Point<int16_t>(0, 13);
								Point<int16_t> pos = cursor_position - cursoroffset;

								if (pos_y.y() <= MIN_HEIGHT)
								{
									temp_view_y = MIN_HEIGHT;
									temp_position = position - Point<int16_t>(0, user_view_y) + Point<int16_t>(0, 13);
								}
								else if (pos_y.y() >= MAX_HEIGHT)
								{
									temp_view_y = MAX_HEIGHT;
									temp_position = position - Point<int16_t>(0, user_view_y) + Point<int16_t>(0, MAX_HEIGHT);
								}
								else
								{
									temp_view_y = pos_y.y();
									temp_position = Point<int16_t>(position.x(), pos.y());
								}
							}
							else
							{
								Point<int16_t> pos_y = position - cursor_position - Point<int16_t>(0, 13);

								if (pos_y.y() <= MIN_HEIGHT)
									temp_view_y = MIN_HEIGHT;
								else if (pos_y.y() >= MAX_HEIGHT)
									temp_view_y = MAX_HEIGHT;
								else
									temp_view_y = pos_y.y();
							}
						}

#if LOG_LEVEL >= LOG_UI
						dimension = Point<int16_t>(temp_view_x, top_y + center_y + bottom_y) + Point<int16_t>(0, temp_view_y);

						dimension_box = ColorBox(dimension.x(), dimension.y(), Color::Name::RED, 0.5f);
						top_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
						bottom_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
						left_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
						right_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
#endif

						if (drag_direction == DragDirection::DOWN || drag_direction == DragDirection::LEFT)
							return Cursor::State::CHATBARBRTLDRAG;
						else
							return Cursor::State::CHATBARBLTRDRAG;
					}
				}
			}
			else if (indragrange(cursor_position))
			{
				cursoroffset = cursor_position - position;
				dragged = true;

				return Cursor::State::CHATBARMOVE;
			}
			else if (view_max)
			{
				if (intoprange(cursor_position) && !inleftrange(cursor_position) && !inrightrange(cursor_position))
				{
					dragged = true;

					temp_view_y = user_view_y;

					return Cursor::State::CHATBARVDRAG;
				}
				else if (inrightrange(cursor_position) && !intoprange(cursor_position) && !inbottomrange(cursor_position))
				{
					dragged = true;

					temp_view_x = user_view_x;

					return Cursor::State::CHATBARHDRAG;
				}
				else if (intoprightrange(cursor_position))
				{
					dragged = true;

					temp_view_x = user_view_x;
					temp_view_y = user_view_y;

					return Cursor::State::CHATBARBLTRDRAG;
				}
				else if (inbottomrange(cursor_position) && !inleftrange(cursor_position) && !inrightrange(cursor_position))
				{
					cursoroffset = cursor_position - position;
					dragged = true;

					temp_view_y = user_view_y;
					temp_position = position;
					drag_direction = DragDirection::DOWN;

					return Cursor::State::CHATBARVDRAG;
				}
				else if (inbottomrightrange(cursor_position))
				{
					cursoroffset = cursor_position - position;
					dragged = true;

					temp_view_x = user_view_x;
					temp_view_y = user_view_y;
					temp_position = position;
					drag_direction = DragDirection::DOWN;

					return Cursor::State::CHATBARBRTLDRAG;
				}
				else if (inleftrange(cursor_position) && !intoprange(cursor_position) && !inbottomrange(cursor_position))
				{
					cursoroffset = cursor_position - position;
					dragged = true;

					temp_view_x = user_view_x;
					temp_position = position;
					drag_direction = DragDirection::LEFT;

					return Cursor::State::CHATBARHDRAG;
				}
				else if (intopleftrange(cursor_position))
				{
					cursoroffset = cursor_position - position;
					dragged = true;

					temp_view_x = user_view_x;
					temp_view_y = user_view_y;
					temp_position = position;
					drag_direction = DragDirection::LEFT;

					return Cursor::State::CHATBARBRTLDRAG;
				}
				else if (inbottomleftrange(cursor_position))
				{
					cursoroffset = cursor_position - position;
					dragged = true;

					temp_view_x = user_view_x;
					temp_view_y = user_view_y;
					temp_position = position;
					drag_direction = DragDirection::DOWNLEFT;

					return Cursor::State::CHATBARBLTRDRAG;
				}
			}
		}
		else
		{
			if (dragged)
			{
				if (temp_view_y == 0 && temp_view_x == 0)
				{
					dragged = false;

					Setting<PosCHAT>::get().save(position);

					return Cursor::State::CHATBARMOVE;
				}
				else
				{
					if (temp_view_x == 0)
					{
						user_view_y = temp_view_y;
						temp_view_y = 0;
						dragged = false;

						update_view(false);

						Setting<ChatViewY>::get().save(user_view_y);

						if (drag_direction == DragDirection::DOWN)
						{
							drag_direction = DragDirection::NONE;

							if (temp_position.y() > min_view_y - input_bg_y)
							{
								if (view_input)
									temp_position.set_y(min_view_y - input_bg_y);
								else
									temp_position.set_y(min_view_y);
							}

							position = temp_position;

							Setting<PosCHAT>::get().save(position);
						}

						return Cursor::State::CHATBARVDRAG;
					}
					else if (temp_view_y == 0)
					{
						user_view_x = temp_view_x;
						temp_view_x = 0;
						dragged = false;

						update_view(false);

						Setting<ChatViewX>::get().save(user_view_x);

						if (drag_direction == DragDirection::LEFT)
						{
							drag_direction = DragDirection::NONE;

							position = temp_position;

							Setting<PosCHAT>::get().save(position);
						}

						return Cursor::State::CHATBARHDRAG;
					}
					else
					{
						user_view_x = temp_view_x;
						temp_view_x = 0;

						user_view_y = temp_view_y;
						temp_view_y = 0;

						dragged = false;

						update_view(false);

						Setting<ChatViewX>::get().save(user_view_x);
						Setting<ChatViewY>::get().save(user_view_y);

						if (drag_direction == DragDirection::DOWN || drag_direction == DragDirection::LEFT || drag_direction == DragDirection::DOWNLEFT)
						{
							drag_direction = DragDirection::NONE;

							if (temp_position.y() > min_view_y - input_bg_y)
							{
								if (view_input)
									temp_position.set_y(min_view_y - input_bg_y);
								else
									temp_position.set_y(min_view_y);
							}

							position = temp_position;

							Setting<PosCHAT>::get().save(position);

							if (drag_direction != DragDirection::DOWNLEFT)
								return Cursor::State::CHATBARBRTLDRAG;
						}

						return Cursor::State::CHATBARBLTRDRAG;
					}
				}
			}
			else if (indragrange(cursor_position))
			{
				return Cursor::State::CHATBARMOVE;
			}
			else if (view_max)
			{
				if (intopleftrange(cursor_position) || inbottomrightrange(cursor_position))
					return Cursor::State::CHATBARBRTLDRAG;
				else if (intoprightrange(cursor_position) || inbottomleftrange(cursor_position))
					return Cursor::State::CHATBARBLTRDRAG;
				else if (intoprange(cursor_position) || inbottomrange(cursor_position))
					return Cursor::State::CHATBARVDRAG;
				else if (inleftrange(cursor_position) || inrightrange(cursor_position))
					return Cursor::State::CHATBARHDRAG;
			}
		}

		return UIElement::send_cursor(clicked, cursor_position);
	}

	void UIChatBar::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (keycode == KeyAction::Id::RETURN)
			{
				if (!view_input)
				{
					toggle_input(true);
				}
				else
				{
					input_text.change_text("");
					input_text.set_state(Textfield::State::FOCUSED);
				}
			}
			else
			{
				int32_t index = UI::get().get_keyboard().get_mapping_index(keycode);

				input_text.change_text("");
				input_text.set_state(Textfield::State::FOCUSED);
				input_text.send_key(KeyType::Id::TEXT, index, pressed);
			}
		}
	}

	UIElement::Type UIChatBar::get_type() const
	{
		return TYPE;
	}

	bool UIChatBar::has_input() const
	{
		return view_input;
	}

	void UIChatBar::toggle_view()
	{
		toggle_view(!view_max, true);
	}

	void UIChatBar::scroll_chat(bool up)
	{
		if (up)
		{
			// Scroll up (show older messages)
			if (chat_scroll_offset < static_cast<int16_t>(message_history.size()) - 1)
				chat_scroll_offset++;
		}
		else
		{
			// Scroll down (show newer messages)
			if (chat_scroll_offset > 0)
				chat_scroll_offset--;
		}
	}

	void UIChatBar::show_message(const char* message, MessageType type)
	{
		Color::Name color = Color::Name::RED;

		if (type == MessageType::YELLOW)
			color = Color::Name::YELLOW;
		else if (type == MessageType::WHITE)
			color = Color::Name::WHITE;
		else
			LOG(LOG_DEBUG, "[UIChatBar::show_message]: " << type << " not supported.");

		message_history.push_back(Message(MessageGroup::ALL, type, Text(Text::Font::A11M, Text::Alignment::LEFT, color, message)));
	}

	bool UIChatBar::indragrange(Point<int16_t> cursor_position) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(get_dragarea_position(), get_dragarea_position() + dragarea);

		return bounds.contains(cursor_position);
	}

	bool UIChatBar::intoprange(Point<int16_t> cursor_position) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(get_position(), get_position() + Point<int16_t>(dimension.x(), 3));

		return bounds.contains(cursor_position);
	}

	bool UIChatBar::inbottomrange(Point<int16_t> cursor_position) const
	{
		Point<int16_t> position = get_position() + Point<int16_t>(0, dimension.y() - 3);
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + Point<int16_t>(dimension.x(), 3));

		return bounds.contains(cursor_position);
	}

	bool UIChatBar::inleftrange(Point<int16_t> cursor_position) const
	{
		Rectangle<int16_t> bounds = Rectangle<int16_t>(get_position(), get_position() + Point<int16_t>(3, dimension.y()));

		return bounds.contains(cursor_position);
	}

	bool UIChatBar::inrightrange(Point<int16_t> cursor_position) const
	{
		Point<int16_t> position = get_position() + Point<int16_t>(dimension.x() - 3, 0);
		Rectangle<int16_t> bounds = Rectangle<int16_t>(position, position + Point<int16_t>(3, dimension.y()));

		return bounds.contains(cursor_position);
	}

	bool UIChatBar::intopleftrange(Point<int16_t> cursor_position) const
	{
		return intoprange(cursor_position) && inleftrange(cursor_position);
	}

	bool UIChatBar::inbottomrightrange(Point<int16_t> cursor_position) const
	{
		return inbottomrange(cursor_position) && inrightrange(cursor_position);
	}

	bool UIChatBar::intoprightrange(Point<int16_t> cursor_position) const
	{
		return intoprange(cursor_position) && inrightrange(cursor_position);
	}

	bool UIChatBar::inbottomleftrange(Point<int16_t> cursor_position) const
	{
		return inbottomrange(cursor_position) && inleftrange(cursor_position);
	}

	Point<int16_t> UIChatBar::get_position() const
	{
		if (temp_view_y == 0 && temp_view_x == 0)
		{
			if (view_max)
				return position - Point<int16_t>(0, top_y + center_y + user_view_y);
			else
				return position - Point<int16_t>(0, top_y + center_y);
		}
		else
		{
			if (temp_view_y > 0 && temp_view_x == 0)
			{
				if (drag_direction == DragDirection::DOWN)
					return temp_position - Point<int16_t>(0, top_y + center_y + temp_view_y);
				else
					return position - Point<int16_t>(0, top_y + center_y + temp_view_y);
			}
			else if (temp_view_y == 0 && temp_view_x > 0)
			{
				if (drag_direction == DragDirection::LEFT)
					return temp_position - Point<int16_t>(0, top_y + center_y + user_view_y);
				else
					return position - Point<int16_t>(0, top_y + center_y + user_view_y);
			}
			else
			{
				if (drag_direction == DragDirection::DOWN || drag_direction == DragDirection::LEFT || drag_direction == DragDirection::DOWNLEFT)
					return temp_position - Point<int16_t>(0, top_y + center_y + temp_view_y);
				else
					return position - Point<int16_t>(0, top_y + center_y + temp_view_y);
			}
		}
	}

	Point<int16_t> UIChatBar::get_dragarea_position() const
	{
		Point<int16_t> drag_origin = drag.get_origin();

		if (view_max)
			return position - Point<int16_t>(drag_origin.x(), top_y + center_y + user_view_y + drag_origin.y());
		else
			return position - Point<int16_t>(drag_origin.x(), top_y + center_y + drag_origin.y());
	}

	Point<int16_t> UIChatBar::get_input_position() const
	{
		return position + Point<int16_t>(0, 15);
	}

	Point<int16_t> UIChatBar::get_input_text_position()
	{
		// Input text should appear at the collapsed chat location, after the channel selector
		// box_top_left = position - (0, CHAT_COLLAPSED_HEIGHT)
		// text_pos = box_top_left + (CHAT_PADDING + channel_width + CHAT_PADDING, centered vertically)
		// Adjusted by (0, -5) for vertical alignment
		Point<int16_t> box_top_left = position - Point<int16_t>(0, CHAT_COLLAPSED_HEIGHT);
		int16_t channel_tex_width = channel_texture.width();
		int16_t input_offset_x = CHAT_PADDING + channel_tex_width + CHAT_PADDING;
		return box_top_left + Point<int16_t>(input_offset_x, (CHAT_COLLAPSED_HEIGHT - 11) / 2 - 5);
	}

	void UIChatBar::toggle_input(bool enabled)
	{
		view_input = enabled;

		// Bring chat to front when enabling input to ensure it renders above statusbar
		if (view_input)
		{
			std::cout << "[DEBUG] UIChatBar::toggle_input - enabling input, calling bring_to_front" << std::endl;
			UI::get().bring_to_front(TYPE);
			input_text.set_state(Textfield::State::FOCUSED);
		}
		else
		{
			input_text.set_state(Textfield::State::DISABLED);
		}

		if (buttons[Buttons::BtChat]) buttons[Buttons::BtChat]->set_active(view_input);
		if (buttons[Buttons::BtItemLink]) buttons[Buttons::BtItemLink]->set_active(view_input);
		if (buttons[Buttons::BtChatEmoticon]) buttons[Buttons::BtChatEmoticon]->set_active(view_input);
		if (buttons[Buttons::BtHelp]) buttons[Buttons::BtHelp]->set_active(view_input);
		if (buttons[Buttons::BtOutChat]) buttons[Buttons::BtOutChat]->set_active(view_input);
	}

	void UIChatBar::toggle_view(bool max, bool pressed)
	{
		view_max = max;

		// Bring chat to front when expanding to ensure it renders above other elements
		if (view_max)
		{
			UI::get().bring_to_front(TYPE);
		}

		if (!view_max)
		{
			view_input = false;
			view_adjusted = false;

			if (buttons[Buttons::BtChat]) buttons[Buttons::BtChat]->set_active(view_input);
			if (buttons[Buttons::BtItemLink]) buttons[Buttons::BtItemLink]->set_active(view_input);
			if (buttons[Buttons::BtChatEmoticon]) buttons[Buttons::BtChatEmoticon]->set_active(view_input);
			if (buttons[Buttons::BtHelp]) buttons[Buttons::BtHelp]->set_active(view_input);
			if (buttons[Buttons::BtOutChat]) buttons[Buttons::BtOutChat]->set_active(view_input);
		}

		Setting<ChatViewMax>::get().save(view_max);

		if (buttons[Buttons::BtMax]) buttons[Buttons::BtMax]->set_active(!view_max);
		if (buttons[Buttons::BtMin]) buttons[Buttons::BtMin]->set_active(view_max);

		// Toggle expand/collapse buttons
		if (buttons[Buttons::BtExpand]) buttons[Buttons::BtExpand]->set_active(!view_max);
		if (buttons[Buttons::BtCollapse]) buttons[Buttons::BtCollapse]->set_active(view_max);

		// Scroll arrows are drawn manually, not as buttons

		update_view(pressed);
	}

	void UIChatBar::update_view(bool pressed)
	{
		if (view_input)
			input_text.set_state(Textfield::State::FOCUSED);
		else
			input_text.set_state(Textfield::State::DISABLED);

		// Chat box position is fixed - no shifting based on view state
		Point<int16_t> btMin_padding = Point<int16_t>(-4, 3);

		if (view_max)
		{
			// Expanded view: 565x150
			dimension = Point<int16_t>(CHAT_WIDTH, CHAT_EXPANDED_HEIGHT);

			if (buttons[Buttons::BtMin]) buttons[Buttons::BtMin]->set_position(Point<int16_t>(user_view_x - btMin_x, -top_y - center_y - user_view_y) + btMin_padding);

			input_btns_pos = Point<int16_t>(input_max_x - (input_bg_x - user_view_x) + input_origin_x - 17, 15 + input_origin_y + 1);

			if (buttons[Buttons::BtChat]) buttons[Buttons::BtChat]->set_position(input_btns_pos + Point<int16_t>(input_btns_x * 0, 0));
			if (buttons[Buttons::BtItemLink]) buttons[Buttons::BtItemLink]->set_position(input_btns_pos + Point<int16_t>(input_btns_x * 1, 0));
			if (buttons[Buttons::BtChatEmoticon]) buttons[Buttons::BtChatEmoticon]->set_position(input_btns_pos + Point<int16_t>(input_btns_x * 2, 0));
			if (buttons[Buttons::BtHelp]) buttons[Buttons::BtHelp]->set_position(input_btns_pos + Point<int16_t>(input_btns_x * 3, 0));
			if (buttons[Buttons::BtOutChat]) buttons[Buttons::BtOutChat]->set_position(input_btns_pos + Point<int16_t>(input_btns_x * 4, 0));
		}
		else
		{
			// Collapsed view: 565x26
			dimension = Point<int16_t>(CHAT_WIDTH, CHAT_COLLAPSED_HEIGHT);

			if (buttons[Buttons::BtMin]) buttons[Buttons::BtMin]->set_position(Point<int16_t>(min_x - btMin_x, -top_y - center_y - user_view_y) + btMin_padding);
		}

#if LOG_LEVEL >= LOG_UI
		dimension_box = ColorBox(dimension.x(), dimension.y(), Color::Name::RED, 0.5f);
		top_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
		bottom_box = ColorBox(dimension.x(), 3, Color::Name::GREEN, 0.5f);
		left_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
		right_box = ColorBox(3, dimension.y(), Color::Name::YELLOW, 0.5f);
		input_box = ColorBox(user_view_x, input_bg_y, Color::Name::RED, 0.5f);
#endif
	}

	void UIChatBar::input_text_enter_callback(std::string message)
	{
		if (message == "")
		{
			input_text_escape_callback();
		}
		else
		{
			user_message_history.push_back(message);
			user_message_history_index = user_message_history.size();

			GeneralChatPacket(message, true).dispatch();

			input_text.change_text("");
		}
	}

	void UIChatBar::input_text_escape_callback()
	{
		toggle_input(false);
	}

	void UIChatBar::change_message(bool up)
	{
		size_t size = user_message_history.size();
		size_t message_history_min = 1;
		size_t message_history_max = size;
		size_t max = 7;

		if (size > max)
			message_history_min = size - max;

		if (user_message_history.size() > 0)
		{
			if (up && user_message_history_index > message_history_min)
				user_message_history_index--;

			if (!up && user_message_history_index < message_history_max)
				user_message_history_index++;

			input_text.change_text(user_message_history[user_message_history_index - 1]);
		}
	}

	std::string UIChatBar::get_chat_target_text() const
	{
		switch (current_chat_target)
		{
			case MessageGroup::ALL:
				return "To All";
			case MessageGroup::PARTY:
				return "To Party";
			case MessageGroup::GUILD:
				return "To Guild";
			case MessageGroup::FRIEND:
				return "To Buddy";
			case MessageGroup::WHISPER:
				return "To Whisper";
			default:
				return "To All";
		}
	}

	void UIChatBar::cycle_chat_target()
	{
		// Cycle through: ALL -> PARTY -> GUILD -> FRIEND -> WHISPER -> ALL
		switch (current_chat_target)
		{
			case MessageGroup::ALL:
				current_chat_target = MessageGroup::PARTY;
				break;
			case MessageGroup::PARTY:
				current_chat_target = MessageGroup::GUILD;
				break;
			case MessageGroup::GUILD:
				current_chat_target = MessageGroup::FRIEND;
				break;
			case MessageGroup::FRIEND:
				current_chat_target = MessageGroup::WHISPER;
				break;
			case MessageGroup::WHISPER:
			default:
				current_chat_target = MessageGroup::ALL;
				break;
		}

		// Update the display text
		channel_text.change_text(get_chat_target_text());
	}
}