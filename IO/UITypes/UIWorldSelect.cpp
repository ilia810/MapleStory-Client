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
#include "UIWorldSelect.h"

#include "UILoginNotice.h"
#include "UILoginWait.h"
#include "UIRegion.h"

#include "../UI.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"

#include "../../Audio/Audio.h"
#include "../../Util/Randomizer.h"
#include "../../Util/Misc.h"

#include "../../Net/Packets/LoginPackets.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	UIWorldSelect::UIWorldSelect() : UIElement(Point<int16_t>(0, 0), Point<int16_t>(800, 600)), worldcount(0), world_selected(false)
	{
		std::string version_text = Configuration::get().get_version();
		version = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);
		version_pos = nl::nx::UI["Login.img"]["Common"]["version"]["pos"];

		worldid = Setting<DefaultWorld>::get().load();
		channelid = Setting<DefaultChannel>::get().load();

		nl::node Login = nl::nx::UI["Login.img"];
		nl::node Common = Login["Common"];
		nl::node WorldSelect = Login["WorldSelect"];

		// Frame/border from Common - stretched to fill 800x600
		if (Common["frame"]) {
			frame = Texture(Common["frame"]);
			frame_stretch = Point<int16_t>(808, 608);
		}

		// Background panel: chBackgrn at (104, 0), size 601x241
		if (WorldSelect["chBackgrn"]) {
			chBackgrn = Texture(WorldSelect["chBackgrn"]);
			chBackgrn_pos = Point<int16_t>(104, 0);
			chBackgrn_stretch = Point<int16_t>(601, 241);
		}

		// BtViewAll button at (0, 29)
		if (WorldSelect["BtViewAll"]) {
			buttons[Buttons::BtViewAll] = std::make_unique<MapleButton>(WorldSelect["BtViewAll"], Point<int16_t>(0, 29));
		}

		// BtExit button at (-1, 520)
		if (Common["BtExit"]) {
			buttons[Buttons::BtExit] = std::make_unique<MapleButton>(Common["BtExit"], Point<int16_t>(-1, 520));
		}

		// Store BtWorld node for dynamic button creation
		BtWorld = WorldSelect["BtWorld"];

		// Channel selection UI (hidden until world selected)
		channelsrc = WorldSelect["BtChannel"];
		channelsrc_pos = Point<int16_t>(314, 217);

		if (channelsrc)
		{
			for (uint16_t i = 0; i < Buttons::BtGoWorld - Buttons::BtChannel0; i++)
			{
				nl::node channelBtn = channelsrc[std::to_string(i)];

				if (channelBtn)
				{
					if (channelBtn["normal"] && channelBtn["normal"]["0"])
					{
						nl::node focusedBtn = channelBtn["keyFocused"] ? channelBtn["keyFocused"]["0"] : channelBtn["normal"]["0"];
						buttons[Buttons::BtChannel0 + i] = std::make_unique<TwoSpriteButton>(channelBtn["normal"]["0"], focusedBtn, channelsrc_pos);
						buttons[Buttons::BtChannel0 + i]->set_active(false);
					}
					else if (channelBtn["0"] && channelBtn["1"])
					{
						buttons[Buttons::BtChannel0 + i] = std::make_unique<TwoSpriteButton>(channelBtn["0"], channelBtn["1"], channelsrc_pos);
						buttons[Buttons::BtChannel0 + i]->set_active(false);
					}
				}

				nl::node gaugeNode = channelsrc["gauge"];
				if (gaugeNode)
				{
					channel_gauge[i] = Gauge(
						Gauge::Type::WORLDSELECT,
						gaugeNode,
						CHANNEL_LOAD,
						0.0f
					);
				}
			}

			channels_background = channelsrc["layer:bg"];

			nl::node goWorldBtn = channelsrc["button:GoWorld"];
			if (goWorldBtn)
			{
				buttons[Buttons::BtGoWorld] = std::make_unique<MapleButton>(goWorldBtn, channelsrc_pos);
				buttons[Buttons::BtGoWorld]->set_active(false);
			}
		}

		for (size_t i = 1; i <= FLAG_SIZE; i++)
			flag_sprites.emplace_back(Login["WorldNotice"][i]);

		worldNotice = WorldSelect["worldNotice"]["0"];
		rebootNotice = WorldSelect["worldNotice"]["reboot"];
		worldNoticeMessage = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::MINESHAFT);
	}

	void UIWorldSelect::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Draw frame stretched to fill 800x600
		if (frame.is_valid()) {
			frame.draw(DrawArgument(
				position + frame.get_origin(),
				Point<int16_t>(0, 0),
				frame_stretch,
				1.0f, 1.0f, 1.0f, 0.0f
			));
		}

		// Draw background panel at (104, 0), stretched to 601x241
		if (chBackgrn.is_valid()) {
			chBackgrn.draw(DrawArgument(
				position + chBackgrn_pos + chBackgrn.get_origin(),
				Point<int16_t>(0, 0),
				chBackgrn_stretch,
				1.0f, 1.0f, 1.0f, 0.0f
			));
		}

		// Draw channel selection if world is selected
		if (world_selected)
		{
			if (channels_background.is_valid())
				channels_background.draw(position + channelsrc_pos);

			if (worldid < world_textures.size() && world_textures[worldid].is_valid())
				world_textures[worldid].draw(position + channelsrc_pos);

			if (worldid < worlds.size())
			{
				uint16_t worldEnum = worlds[worldid].id;

				if (worldEnum == Worlds::REBOOT0 && rebootNotice.is_valid())
				{
					rebootNotice.draw(position);
				}
				else if (!worldNoticeMessage.get_text().empty())
				{
					worldNotice.draw(position + Point<int16_t>(-18, 3));
					worldNoticeMessage.draw(position + channelsrc_pos + Point<int16_t>(58, -50));
				}
			}
		}

		UIElement::draw_buttons(alpha);

		// Draw channel gauges if world selected
		if (world_selected)
		{
			World selectedWorld = worlds[worldid];
			uint8_t channel_total = selectedWorld.channel_count;

			for (size_t i = 0; i < Buttons::BtGoWorld - Buttons::BtChannel0 && i < channel_total; i++)
			{
				uint8_t columns = std::min(channel_total, COLUMNS);

				div_t div = std::div(i, columns);
				int32_t current_col = div.rem;
				int32_t current_row = div.quot;

				channel_gauge->draw(position + channelsrc_pos + Point<int16_t>(28 + 71 * current_col, 92 + 30 * current_row));
			}
		}

		version.draw(position + version_pos - Point<int16_t>(0, 5));
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Rectangle<int16_t> channels_bounds = Rectangle<int16_t>(
			position + channelsrc_pos,
			position + channelsrc_pos + channels_background.get_dimensions()
		);

		if (world_selected && !channels_bounds.contains(cursorpos))
		{
			if (clicked)
			{
				world_selected = false;
				clear_selected_world();
			}
		}

		Cursor::State ret = clicked ? Cursor::State::CLICKING : Cursor::State::IDLE;

		for (auto& btit : buttons)
		{
			if (btit.second->is_active() && btit.second->bounds(position).contains(cursorpos))
			{
				if (btit.second->get_state() == Button::State::NORMAL)
				{
					Sound(Sound::Name::BUTTONOVER).play();

					btit.second->set_state(Button::State::MOUSEOVER);
					ret = Cursor::State::CANCLICK;
				}
				else if (btit.second->get_state() == Button::State::PRESSED)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();

						btit.second->set_state(button_pressed(btit.first));

						ret = Cursor::State::IDLE;
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
				else if (btit.second->get_state() == Button::State::MOUSEOVER)
				{
					if (clicked)
					{
						Sound(Sound::Name::BUTTONCLICK).play();

						btit.second->set_state(button_pressed(btit.first));

						ret = Cursor::State::IDLE;
					}
					else
					{
						ret = Cursor::State::CANCLICK;
					}
				}
			}
			else if (btit.second->get_state() == Button::State::MOUSEOVER)
			{
				btit.second->set_state(Button::State::NORMAL);
			}
		}

		return ret;
	}

	void UIWorldSelect::send_key(int32_t keycode, bool pressed, bool escape)
	{
		if (pressed)
		{
			if (world_selected)
			{
				World selectedWorld = worlds[worldid];

				uint8_t selected_channel = channelid;
				uint8_t channel_total = selectedWorld.channel_count;

				uint8_t columns = std::min(channel_total, COLUMNS);

				uint8_t rows = std::floor((channel_total - 1) / COLUMNS) + 1;

				div_t div = std::div(selected_channel, columns);
				int32_t current_col = div.rem;

				if (keycode == KeyAction::Id::UP)
				{
					auto next_channel = (selected_channel - COLUMNS < 0 ? (selected_channel - COLUMNS) + rows * COLUMNS : selected_channel - COLUMNS);

					if (next_channel == channelid)
						return;

					if (next_channel > channel_total)
						button_pressed(next_channel - COLUMNS + Buttons::BtChannel0);
					else
						button_pressed(next_channel + Buttons::BtChannel0);
				}
				else if (keycode == KeyAction::Id::DOWN)
				{
					auto next_channel = (selected_channel + COLUMNS >= channel_total ? current_col : selected_channel + COLUMNS);

					if (next_channel == channelid)
						return;

					if (next_channel > channel_total)
						button_pressed(next_channel + COLUMNS + Buttons::BtChannel0);
					else
						button_pressed(next_channel + Buttons::BtChannel0);
				}
				else if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::TAB)
				{
					if (selected_channel != 0)
						selected_channel--;
					else
						selected_channel = channel_total - 1;

					button_pressed(selected_channel + Buttons::BtChannel0);
				}
				else if (keycode == KeyAction::Id::RIGHT)
				{
					if (selected_channel != channel_total - 1)
						selected_channel++;
					else
						selected_channel = 0;

					button_pressed(selected_channel + Buttons::BtChannel0);
				}
				else if (escape)
				{
					world_selected = false;

					clear_selected_world();
				}
				else if (keycode == KeyAction::Id::RETURN)
				{
					button_pressed(Buttons::BtGoWorld);
				}
			}
			else
			{
				if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::RIGHT || keycode == KeyAction::Id::TAB)
				{
					if (worlds.empty())
						return;

					// Deactivate old world button
					if (worldid < worlds.size())
					{
						auto old_btn = buttons.find(Buttons::BtWorld0 + worldid);
						if (old_btn != buttons.end() && old_btn->second)
							old_btn->second->set_state(Button::State::NORMAL);
					}

					// Navigate through worlds array
					bool forward = keycode == KeyAction::Id::RIGHT;
					if (forward)
						worldid = (worldid + 1) % worlds.size();
					else
						worldid = (worldid == 0) ? static_cast<uint8_t>(worlds.size() - 1) : worldid - 1;

					// Activate new world button
					auto new_btn = buttons.find(Buttons::BtWorld0 + worldid);
					if (new_btn != buttons.end() && new_btn->second)
						new_btn->second->set_state(Button::State::PRESSED);
				}
				else if (escape)
				{
					auto quitconfirm = UI::get().get_element<UIQuitConfirm>();

					if (quitconfirm && quitconfirm->is_active())
						return UI::get().send_key(keycode, pressed);
					else
						button_pressed(Buttons::BtExit);
				}
				else if (keycode == KeyAction::Id::RETURN)
				{
					auto quitconfirm = UI::get().get_element<UIQuitConfirm>();

					if (quitconfirm && quitconfirm->is_active())
					{
						return UI::get().send_key(keycode, pressed);
					}
					else
					{
						// Press the current world button
						if (worldid < worlds.size())
							button_pressed(Buttons::BtWorld0 + worldid);
					}
				}
			}
		}
	}

	UIElement::Type UIWorldSelect::get_type() const
	{
		return TYPE;
	}

	void UIWorldSelect::draw_world()
	{
		if (worldcount <= 0)
			return;

		// Create world buttons dynamically based on available worlds
		// Layout: horizontal at y=43, starting x=124, spacing=95
		const int16_t START_X = 124;
		const int16_t START_Y = 43;
		const int16_t SPACING_X = 95;

		for (size_t i = 0; i < worlds.size(); i++)
		{
			World& world = worlds[i];

			if (world.channel_count < 2)
				continue;

			// Calculate position for this world button
			Point<int16_t> btnPos(START_X + static_cast<int16_t>(i) * SPACING_X, START_Y);

			// Get world texture ID (use world.id to find the right texture)
			std::string texture_id = std::to_string(world.id);
			nl::node worldBtn = BtWorld[texture_id];

			// Fallback to generic "e" texture if specific one not found
			if (!worldBtn || !worldBtn["normal"])
				worldBtn = BtWorld["e"];

			if (worldBtn && worldBtn["normal"] && worldBtn["normal"]["0"])
			{
				nl::node normalBtn = worldBtn["normal"]["0"];
				nl::node pressedBtn = worldBtn["pressed"] ? worldBtn["pressed"]["0"] : normalBtn;
				nl::node mouseOverBtn = worldBtn["mouseOver"] ? worldBtn["mouseOver"]["0"] : normalBtn;

				buttons[Buttons::BtWorld0 + static_cast<uint16_t>(i)] = std::make_unique<TwoSpriteButton>(normalBtn, mouseOverBtn, btnPos);
				buttons[Buttons::BtWorld0 + static_cast<uint16_t>(i)]->set_active(true);
			}

			if (channelid >= world.channel_count)
				channelid = 0;
		}

		// Auto-select world if enabled
		if (Configuration::get().get_auto_login())
		{
			LOG(LOG_DEBUG, "[UIWorldSelect] Auto-login enabled, attempting auto-world selection");

			uint8_t auto_world_id = Configuration::get().get_auto_world();
			uint8_t auto_channel_id = Configuration::get().get_auto_channel();

			// Find the index in worlds[] that matches auto_world_id
			bool world_exists = false;
			for (size_t i = 0; i < worlds.size(); i++)
			{
				if (worlds[i].id == auto_world_id)
				{
					worldid = static_cast<uint8_t>(i);
					world_exists = true;
					break;
				}
			}

			if (world_exists)
			{
				LOG(LOG_DEBUG, "[UIWorldSelect] Auto-world found, selecting world " + std::to_string(auto_world_id) + " channel " + std::to_string(auto_channel_id));
				channelid = auto_channel_id;

				enter_world();
			}
			else
			{
				LOG(LOG_ERROR, "[UIWorldSelect] Auto-world ID " + std::to_string(auto_world_id) + " not found in world list");
			}
		}
	}

	void UIWorldSelect::add_world(World world)
	{
		worlds.emplace_back(std::move(world));
		worldcount++;
	}

	void UIWorldSelect::change_world(World selectedWorld)
	{
		// Activate the selected world button
		for (size_t i = 0; i < worlds.size(); i++)
		{
			if (worlds[i].id == selectedWorld.id)
			{
				auto world_btn = buttons.find(Buttons::BtWorld0 + i);
				if (world_btn != buttons.end() && world_btn->second)
					world_btn->second->set_state(Button::State::PRESSED);
				break;
			}
		}

		for (size_t i = 0; i < selectedWorld.channel_count; ++i)
		{
			auto ch_btn = buttons.find(Buttons::BtChannel0 + i);
			if (ch_btn != buttons.end() && ch_btn->second)
			{
				ch_btn->second->set_active(true);

				if (i == channelid)
					ch_btn->second->set_state(Button::State::PRESSED);
			}

			channel_gauge[i].update(selectedWorld.channel_capacities[i]);
		}

		auto go_btn = buttons.find(Buttons::BtGoWorld);
		if (go_btn != buttons.end() && go_btn->second)
			go_btn->second->set_active(true);

		worldNoticeMessage.change_text(selectedWorld.event_message);
	}

	void UIWorldSelect::remove_selected()
	{
		deactivate();

		Sound(Sound::Name::SCROLLUP).play();

		world_selected = false;

		clear_selected_world();
	}

	void UIWorldSelect::set_region(uint8_t regionid)
	{
		worldsrc_pos = Point<int16_t>(0, 0);
	}

	uint16_t UIWorldSelect::get_worldbyid(uint16_t worldid)
	{
		if (worldid < worlds.size())
			return worlds[worldid].id;
		return Worlds::SCANIA;
	}

	Button::State UIWorldSelect::button_pressed(uint16_t id)
	{
		if (id == Buttons::BtGoWorld)
		{
			enter_world();

			return Button::State::NORMAL;
		}
		else if (id == Buttons::BtExit)
		{
			UI::get().emplace<UIQuitConfirm>();

			return Button::State::NORMAL;
		}
		else if (id == Buttons::BtViewAll)
		{
			// View all worlds - no action needed for now
			return Button::State::NORMAL;
		}
		else if (id >= Buttons::BtWorld0 && id < Buttons::BtChannel0)
		{
			uint16_t world_index = id - Buttons::BtWorld0;

			// Deactivate old world button
			if (worldid < worlds.size())
			{
				auto old_world_btn = buttons.find(Buttons::BtWorld0 + worldid);
				if (old_world_btn != buttons.end() && old_world_btn->second)
					old_world_btn->second->set_state(Button::State::NORMAL);
			}

			// Set new world index
			if (world_index < worlds.size())
			{
				worldid = static_cast<uint8_t>(world_index);

				// Send server status request with actual world ID
				ServerStatusRequestPacket(worlds[worldid].id).dispatch();

				auto go_world_btn = buttons.find(Buttons::BtGoWorld);
				if (!channelsrc || go_world_btn == buttons.end() || !go_world_btn->second)
				{
					channelid = 0;
					enter_world();
					return Button::State::NORMAL;
				}

				world_selected = true;
				clear_selected_world();
				change_world(worlds[worldid]);

				return Button::State::PRESSED;
			}

			return Button::State::NORMAL;
		}
		else if (id >= Buttons::BtChannel0 && id < Buttons::BtGoWorld)
		{
			uint8_t selectedch = static_cast<uint8_t>(id - Buttons::BtChannel0);

			if (selectedch != channelid)
			{
				auto old_ch_btn = buttons.find(Buttons::BtChannel0 + channelid);
				if (old_ch_btn != buttons.end() && old_ch_btn->second)
					old_ch_btn->second->set_state(Button::State::NORMAL);
				channelid = static_cast<uint8_t>(id - Buttons::BtChannel0);
				auto new_ch_btn = buttons.find(Buttons::BtChannel0 + channelid);
				if (new_ch_btn != buttons.end() && new_ch_btn->second)
					new_ch_btn->second->set_state(Button::State::PRESSED);
				Sound(Sound::Name::WORLDSELECT).play();
			}
			else
			{
				enter_world();
			}

			return Button::State::PRESSED;
		}
		else
		{
			return Button::State::NORMAL;
		}
	}

	void UIWorldSelect::enter_world()
	{
		LOG(LOG_DEBUG, "[UIWorldSelect] enter_world() called");

		// Get the actual world ID from the selected world in the array
		uint8_t actual_world_id = 0;
		if (worldid < worlds.size())
			actual_world_id = worlds[worldid].id;

		LOG(LOG_DEBUG, "[UIWorldSelect] Setting world ID: " + std::to_string(actual_world_id) + " channel ID: " + std::to_string(channelid));

		Configuration::get().set_worldid(actual_world_id);
		Configuration::get().set_channelid(channelid);

		LOG(LOG_DEBUG, "[UIWorldSelect] Creating UILoginWait");
		UI::get().emplace<UILoginWait>();
		auto loginwait = UI::get().get_element<UILoginWait>();

		if (loginwait && loginwait->is_active())
		{
			LOG(LOG_DEBUG, "[UIWorldSelect] Sending CharlistRequestPacket");
			CharlistRequestPacket(actual_world_id, channelid).dispatch();
		}
		else
		{
			LOG(LOG_ERROR, "[UIWorldSelect] UILoginWait not active or not found");
		}
	}

	void UIWorldSelect::clear_selected_world()
	{
		channelid = 0;

		for (size_t i = Buttons::BtChannel0; i < Buttons::BtGoWorld; i++)
		{
			auto btn = buttons.find(i);
			if (btn != buttons.end() && btn->second)
				btn->second->set_state(Button::State::NORMAL);
		}

		auto ch0_btn = buttons.find(Buttons::BtChannel0);
		if (ch0_btn != buttons.end() && ch0_btn->second)
			ch0_btn->second->set_state(Button::State::PRESSED);

		for (size_t i = 0; i < Buttons::BtGoWorld - Buttons::BtChannel0; i++)
		{
			auto btn = buttons.find(Buttons::BtChannel0 + i);
			if (btn != buttons.end() && btn->second)
			{
				btn->second->set_active(false);
				channel_gauge[i].update(0);
			}
		}

		auto go_btn = buttons.find(Buttons::BtGoWorld);
		if (go_btn != buttons.end() && go_btn->second)
			go_btn->second->set_active(false);

		worldNoticeMessage.change_text("");
	}

}
