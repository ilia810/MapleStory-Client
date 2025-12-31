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
#pragma once

#include "../UIDragElement.h"

#include "../Components/Textfield.h"

// TODO: Change these?
#include "../Messages.h"

namespace ms
{
	class UIChatBar : public UIDragElement<PosCHAT>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CHATBAR;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;

		UIChatBar();

		void draw(float inter) const override;
		void update() override;

		Button::State button_pressed(uint16_t buttonid) override;

		bool is_in_range(Point<int16_t> cursor_position) const override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursor_position) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		enum MessageGroup : uint8_t
		{
			ALL,
			BATTLE,
			PARTY,
			FRIEND,
			GUILD,
			ALLIANCE,
			SPOUSE,
			GROUP,
			WHISPER,
			LENGTH
		};

		enum MessageType : uint8_t
		{
			UNK0,
			WHITE,
			RED,
			BLUE,
			YELLOW
		};

		bool has_input() const;
		void toggle_view();
		void show_message(const char* message, MessageType type);

		// TODO: Change these?
		void change_target(int32_t action) {}

	private:
		static constexpr int16_t INPUT_TEXT_HEIGHT = 18;
		static constexpr int16_t MIN_HEIGHT = 12;
		static constexpr int16_t MAX_HEIGHT = 467;

		// New chat box dimensions
		static constexpr int16_t CHAT_WIDTH = 565;
		static constexpr int16_t CHAT_COLLAPSED_HEIGHT = 26;
		static constexpr int16_t CHAT_EXPANDED_HEIGHT = 150;
		static constexpr int16_t CHAT_PADDING = 5;

		bool indragrange(Point<int16_t> cursor_position) const override;
		bool intoprange(Point<int16_t> cursor_position) const;
		bool inbottomrange(Point<int16_t> cursor_position) const;
		bool inleftrange(Point<int16_t> cursor_position)  const;
		bool inrightrange(Point<int16_t> cursor_position) const;
		bool intopleftrange(Point<int16_t> cursor_position)  const;
		bool inbottomrightrange(Point<int16_t> cursor_position) const;
		bool intoprightrange(Point<int16_t> cursor_position)  const;
		bool inbottomleftrange(Point<int16_t> cursor_position) const;

		Point<int16_t> get_position() const;
		Point<int16_t> get_dragarea_position() const;
		Point<int16_t> get_input_position() const;
		Point<int16_t> get_input_text_position();
		void toggle_input(bool enabled);
		void toggle_view(bool max, bool pressed);
		void update_view(bool pressed);
		void input_text_enter_callback(std::string message);
		void input_text_escape_callback();
		void change_message(bool up);
		void scroll_chat(bool up);  // Scroll chat messages up or down
		std::string get_chat_target_text() const;  // Get display text for current chat target
		void cycle_chat_target();  // Cycle to next chat target

		enum Buttons
		{
			BtMax,
			BtMin,
			BtChat,
			BtHelp,
			BtItemLink,
			BtChatEmoticon,
			BtOutChat,
			BtExpand,    // Expand button (shown when collapsed)
			BtCollapse,  // Collapse button (shown when expanded)
			BtScrollUp,  // Scroll up arrow (collapsed view)
			BtScrollDown // Scroll down arrow (collapsed view)
		};

		enum DragDirection
		{
			NONE,
			DOWN,
			LEFT,
			DOWNLEFT
		};

		Texture drag;
		std::vector<Texture> max_textures;
		std::vector<Texture> min_textures;
		std::vector<Texture> input_textures;

		// Scrollbar/scroll arrow textures (VScr4)
		Texture scrollbar_base;
		Texture scrollbar_thumb;
		Texture arrow_up_tex;
		Texture arrow_down_tex;

		// Chat box background (simple colored box)
		int16_t chat_scroll_offset;  // Current scroll position in message history

		int16_t min_x;
		int16_t max_x;
		int16_t top_y;
		int16_t center_y;
		int16_t bottom_y;
		int16_t btMin_x;
		int16_t user_view_x;
		int16_t user_view_y;
		int16_t temp_view_x;
		int16_t temp_view_y;
		int16_t min_view_y;
		bool view_max;
		bool view_input;
		bool view_adjusted;
		bool position_adjusted;
		DragDirection drag_direction;
		Point<int16_t> temp_position;

		int16_t input_bg_x;
		int16_t input_bg_y;
		int16_t input_max_x;
		int16_t input_origin_x;
		int16_t input_origin_y;
		int16_t input_btns_x;
		Point<int16_t> input_btns_pos;
		Textfield input_text;

		struct Message
		{
			MessageGroup group;
			MessageType type;
			Text text;

			Message() : group(MessageGroup::ALL), type(MessageType::UNK0), text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, "") {}
			Message(std::string text) : group(MessageGroup::ALL), type(MessageType::UNK0), text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::WHITE, text) {}
			Message(MessageGroup group, MessageType type, Text text) : group(group), type(type), text(text) {}
		};

		std::vector<Message> message_history;
		std::vector<std::string> user_message_history;
		size_t user_message_history_index;

		// Chat target selector (channel2 texture)
		Texture channel_texture;
		Text channel_text;
		MessageGroup current_chat_target;

#if LOG_LEVEL >= LOG_UI
		ColorBox dimension_box;
		ColorBox dragarea_box;
		ColorBox top_box;
		ColorBox bottom_box;
		ColorBox left_box;
		ColorBox right_box;
		ColorBox input_box;
#endif
	};
}