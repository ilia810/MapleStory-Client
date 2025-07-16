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
#include "../../Util/V83UIAssets.h"

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

		Point<int16_t> background_pos = Point<int16_t>(512, 384);
		channelsrc_pos = Point<int16_t>(314, 217);

		worldid = Setting<DefaultWorld>::get().load();
		channelid = Setting<DefaultChannel>::get().load();

		nl::node Login = nl::nx::UI["Login.img"];
		nl::node Common = Login["Common"];
		nl::node WorldSelect = Login["WorldSelect"];
		
		// Use V83 compatibility layer
		worldsrc = V83UIAssets::getWorldButton();
		channelsrc = WorldSelect["BtChannel"];
		
		if (!worldsrc) {
			LOG(LOG_ERROR, "[UIWorldSelect] World button not found");
		}
		
		if (!channelsrc) {
			LOG(LOG_DEBUG, "[UIWorldSelect] Channel buttons not found (v83 mode)");
		}

		uint8_t regionid = Setting<DefaultRegion>::get().load();
		set_region(regionid);

		// Use V83 compatibility layer for background
		nl::node bg_node = V83UIAssets::getWorldSelectBackground();
		if (bg_node)
		{
			sprites.emplace_back(bg_node, background_pos);

			std::vector<std::string> backgrounds = { "MapleLive" };
			size_t backgrounds_size = backgrounds.size();

			if (backgrounds_size > 0)
			{
				// Try to load additional background variants
				nl::node WorldSelectMap = nl::nx::Map["Obj"]["login.img"]["WorldSelect"];
				if (WorldSelectMap)
				{
					if (backgrounds_size > 1)
					{
						Randomizer randomizer = Randomizer();
						size_t index = randomizer.next_int(backgrounds_size);

						if (WorldSelectMap[backgrounds[index]]["0"])
							sprites.emplace_back(WorldSelectMap[backgrounds[index]]["0"], background_pos);
					}
					else
					{
						if (WorldSelectMap[backgrounds[0]]["0"])
							sprites.emplace_back(WorldSelectMap[backgrounds[0]]["0"], background_pos);
					}
				}
			}
		}
		else
		{
			// v87 fallback: try to find background in UI data
			nl::node uiBackground = WorldSelect["background"];
			if (!uiBackground)
				uiBackground = Login["background"];
			
			if (uiBackground)
				sprites.emplace_back(uiBackground, background_pos);
			// If no background found, continue without one (better than crashing)
		}

		// Only create region button if it exists (might not be in v87)
		if (WorldSelect["BtRegion"])
			buttons[Buttons::BtRegion] = std::make_unique<MapleButton>(WorldSelect["BtRegion"], Point<int16_t>(0, 1));
		
		buttons[Buttons::BtExit] = std::make_unique<MapleButton>(Common["BtExit"]);

		// Setup channel buttons with V83 compatibility
		if (channelsrc)
		{
			for (uint16_t i = 0; i < Buttons::BtGoWorld - Buttons::BtChannel0; i++)
			{
				nl::node channelBtn = V83UIAssets::getChannelButton(i);
				if (!channelBtn) {
					// Try alternative path
					channelBtn = channelsrc["button:" + std::to_string(i)];
				}
				
				if (channelBtn)
				{
					// Check for modern structure
					if (channelBtn["normal"]["0"])
					{
						nl::node focusedBtn = channelBtn["keyFocused"]["0"];
						if (!focusedBtn) 
							focusedBtn = channelBtn["normal"]["0"]; // fallback to normal if focused missing

						buttons[Buttons::BtChannel0 + i] = std::make_unique<TwoSpriteButton>(channelBtn["normal"]["0"], focusedBtn, channelsrc_pos);
						buttons[Buttons::BtChannel0 + i]->set_active(false);
					}
					// Check for v83 simple structure
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
		else
		{
			LOG(LOG_DEBUG, "[UIWorldSelect] No channel UI in v83 mode - will auto-select channel");
		}

		chatballoon.change_text("Please select the World you would like to play in.");

		for (size_t i = 1; i <= FLAG_SIZE; i++)
			flag_sprites.emplace_back(Login["WorldNotice"][i]);

		worldNotice = WorldSelect["worldNotice"]["0"];
		rebootNotice = WorldSelect["worldNotice"]["reboot"];
		worldNoticeMessage = Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::MINESHAFT);

		// Don't auto-enter world in constructor - wait for draw_world() to be called
		// when the server list is received
	}

	void UIWorldSelect::draw(float alpha) const
	{
		UIElement::draw_sprites(alpha);

		// Only draw worlds background if it exists
		if (worlds_background.is_valid())
			worlds_background.draw(position + worldsrc_pos);

		for (size_t i = 0; i < worlds.size(); i++)
		{
			World world = worlds[i];

			if (world.flag > 0 && world.flag <= FLAG_SIZE && i < flag_sprites.size())
				flag_sprites[world.flag - 1].draw(position + worldsrc_pos + Point<int16_t>(1, 25 + 33 * i), alpha);
		}

		if (world_selected)
		{
			// Only draw channel UI if it exists (not in v87)
			if (channels_background.is_valid())
				channels_background.draw(position + channelsrc_pos);
			
			// Only draw world texture if available
			if (worldid < world_textures.size() && world_textures[worldid].is_valid())
				world_textures[worldid].draw(position + channelsrc_pos);

			uint16_t worldEnum = world_map.find(worldid)->second;

			// Skip Reboot notice for v87 (no Reboot worlds)
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

		UIElement::draw_buttons(alpha);

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
		chatballoon.draw(position + Point<int16_t>(747, 105));
	}

	Cursor::State UIWorldSelect::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		Rectangle<int16_t> channels_bounds = Rectangle<int16_t>(
			position + channelsrc_pos,
			position + channelsrc_pos + channels_background.get_dimensions()
			);

		Rectangle<int16_t> worlds_bounds = Rectangle<int16_t>(
			position + worldsrc_pos,
			position + worldsrc_pos + worlds_background.get_dimensions()
			);

		if (world_selected && !channels_bounds.contains(cursorpos) && !worlds_bounds.contains(cursorpos))
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
				auto selected_world = worldid;
				auto world_count = worldcount - 1;

				if (keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::RIGHT || keycode == KeyAction::Id::UP || keycode == KeyAction::Id::DOWN || keycode == KeyAction::Id::TAB)
				{
					bool world_found = false;
					bool forward = keycode == KeyAction::Id::LEFT || keycode == KeyAction::Id::UP;

					while (!world_found)
					{
						selected_world = get_next_world(selected_world, forward);

						for (auto world : worlds)
						{
							if (world.id == selected_world)
							{
								world_found = true;
								break;
							}
						}
					}

					buttons[Buttons::BtWorld0 + worldid]->set_state(Button::State::NORMAL);

					worldid = static_cast<uint8_t>(selected_world);

					buttons[Buttons::BtWorld0 + worldid]->set_state(Button::State::PRESSED);
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
						bool found = false;

						for (size_t i = Buttons::BtWorld0; i < Buttons::BtChannel0; i++)
						{
							auto state = buttons[Buttons::BtWorld0 + i]->get_state();

							if (state == Button::State::PRESSED)
							{
								found = true;
								break;
							}
						}

						if (found)
							button_pressed(selected_world + Buttons::BtWorld0);
						else
							buttons[Buttons::BtWorld0 + selected_world]->set_state(Button::State::PRESSED);
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
			return; // TODO: Send the user back to the login screen? Otherwise, I think the screen will be blank with no worlds, or throw a UILoginNotice up with failed to communite to server?

		for (auto world : worlds)
		{
			if (world.channel_count < 2)
				return; // I remove the world if there is only one channel because the graphic for the channel selection is defaulted to at least 2

			buttons[Buttons::BtWorld0 + world.id]->set_active(true);

			if (channelid >= world.channel_count)
				channelid = 0;
		}

		// Auto-select world if enabled
		if (Configuration::get().get_auto_login())
		{
			LOG(LOG_DEBUG, "[UIWorldSelect] Auto-login enabled, attempting auto-world selection");
			
			uint8_t auto_world_id = Configuration::get().get_auto_world();
			uint8_t auto_channel_id = Configuration::get().get_auto_channel();
			
			LOG(LOG_DEBUG, "[UIWorldSelect] Auto-world ID: " + std::to_string(auto_world_id));
			LOG(LOG_DEBUG, "[UIWorldSelect] Auto-channel ID: " + std::to_string(auto_channel_id));
			LOG(LOG_DEBUG, "[UIWorldSelect] Number of worlds: " + std::to_string(worlds.size()));
			
			// Check if the auto world exists
			bool world_exists = false;
			for (auto world : worlds)
			{
				LOG(LOG_DEBUG, "[UIWorldSelect] Checking world ID: " + std::to_string(world.id) + " Name: " + world.name);
				if (world.id == auto_world_id)
				{
					world_exists = true;
					break;
				}
			}
			
			if (world_exists)
			{
				LOG(LOG_DEBUG, "[UIWorldSelect] Auto-world found, selecting world " + std::to_string(auto_world_id) + " channel " + std::to_string(auto_channel_id));
				worldid = auto_world_id;
				channelid = auto_channel_id;
				
				// Auto-enter world
				LOG(LOG_DEBUG, "[UIWorldSelect] Calling enter_world() for auto-login");
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
		buttons[Buttons::BtWorld0 + selectedWorld.id]->set_state(Button::State::PRESSED);

		for (size_t i = 0; i < selectedWorld.channel_count; ++i)
		{
			buttons[Buttons::BtChannel0 + i]->set_active(true);

			if (i == channelid)
				buttons[Buttons::BtChannel0 + i]->set_state(Button::State::PRESSED);

			channel_gauge[i].update(selectedWorld.channel_capacities[i]);
		}

		buttons[Buttons::BtGoWorld]->set_active(true);

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
		// Set up v87-compatible world mapping (no Reboot worlds in v87)
		world_map[Buttons::BtWorld0] = Worlds::SCANIA;
		world_map[Buttons::BtWorld1] = Worlds::BERA;
		world_map[Buttons::BtWorld2] = Worlds::BROA;
		world_map[Buttons::BtWorld3] = Worlds::WINDIA;
		world_map[Buttons::BtWorld4] = Worlds::KHAINI;

		// Try modern UI structure first
		nl::node region = worldsrc["index"][regionid];
		if (region && region["layer:bg"])
		{
			// Modern UI with region support
			worlds_background = region["layer:bg"];
			worldsrc_pos = region["pos"];

			for (uint16_t i = Buttons::BtWorld0; i < Buttons::BtChannel0; i++)
			{
				std::string world = std::to_string(world_map[i]);
				
				// v92 doesn't have release/layer structure for selected worlds
				// Try different paths for selected world textures
				if (V83UIAssets::isV92Mode()) {
					// v92 structure: scroll animations for selected world
					nl::node scrollNode = nl::nx::UI["Login.img"]["WorldSelect"]["scroll"];
					if (scrollNode && scrollNode[world]) {
						nl::node worldScroll = scrollNode[world];
						if (worldScroll["0"]) {
							// Use the first frame of the scroll animation
							world_textures.emplace_back(worldScroll["0"]);
							LOG(LOG_DEBUG, "[UIWorldSelect] Found v92 world texture at scroll/" << world << "/0");
						} else {
							// No animation frame found
							world_textures.emplace_back();
							LOG(LOG_DEBUG, "[UIWorldSelect] No v92 world texture found for world " << world);
						}
					} else {
						world_textures.emplace_back();
					}
				} else if (channelsrc) {
					// Non-v92 structure
					if (channelsrc["release"] && channelsrc["release"]["layer:" + world]) {
						// Modern structure
						world_textures.emplace_back(channelsrc["release"]["layer:" + world]);
					} else {
						// No selected world texture - use empty texture
						world_textures.emplace_back();
					}
				} else {
					world_textures.emplace_back();
				}

				nl::node worldbtn = worldsrc["button:" + world];
				if (worldbtn && worldbtn["normal"]["0"])
				{
					nl::node focusedBtn = worldbtn["keyFocused"]["0"];
					if (!focusedBtn) focusedBtn = worldbtn["normal"]["0"];

					buttons[Buttons::BtWorld0 + i] = std::make_unique<TwoSpriteButton>(worldbtn["normal"]["0"], focusedBtn, worldsrc_pos + Point<int16_t>(region["origin"][i + 1]));
					buttons[Buttons::BtWorld0 + i]->set_active(false);
				}
			}
		}
		else
		{
			// v87 fallback: simpler world structure
			if (worldsrc && worldsrc["layer:bg"])
				worlds_background = worldsrc["layer:bg"];
			
			worldsrc_pos = Point<int16_t>(50, 150); // Default position for v87

			// v87 likely has direct world buttons (world0, world1, etc.)
			for (uint16_t i = Buttons::BtWorld0; i < Buttons::BtChannel0; i++)
			{
				std::string worldIndex = std::to_string(i);
				nl::node worldbtn = worldsrc[worldIndex]; // Try indexed access
				
				if (!worldbtn) // Try named access
				{
					std::string worldName = std::to_string(world_map[i]);
					worldbtn = worldsrc[worldName];
				}
				
				if (worldbtn && worldbtn["normal"])
				{
					nl::node normalBtn = worldbtn["normal"][0] ? worldbtn["normal"][0] : worldbtn["normal"];
					nl::node focusedBtn = worldbtn["mouseOver"] ? worldbtn["mouseOver"][0] : normalBtn;
					if (!focusedBtn) focusedBtn = normalBtn;

					// Position worlds vertically for v87
					Point<int16_t> worldPos = worldsrc_pos + Point<int16_t>(0, i * 35);
					buttons[Buttons::BtWorld0 + i] = std::make_unique<TwoSpriteButton>(normalBtn, focusedBtn, worldPos);
					buttons[Buttons::BtWorld0 + i]->set_active(false);
				}
			}
		}
	}

	uint16_t UIWorldSelect::get_worldbyid(uint16_t worldid)
	{
		return world_map.find(worldid)->second;
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
		else if (id == Buttons::BtRegion)
		{
			UI::get().emplace<UIRegion>();

			deactivate();

			return Button::State::NORMAL;
		}
		else if (id >= Buttons::BtWorld0 && id < Buttons::BtChannel0)
		{
			buttons[Buttons::BtWorld0 + worldid]->set_state(Button::State::NORMAL);

			worldid = id - Buttons::BtWorld0;

			ServerStatusRequestPacket(worldid).dispatch();

			// If no modern channel UI exists (v87), auto-enter world with channel 1
			if (!channelsrc || !buttons[Buttons::BtGoWorld])
			{
				channelid = 1; // Default to channel 1 for v87
				enter_world(); // Enter immediately without channel selection
				return Button::State::NORMAL;
			}

			world_selected = true;
			clear_selected_world();
			change_world(worlds[worldid]);

			return Button::State::PRESSED;
		}
		else if (id >= Buttons::BtChannel0 && id < Buttons::BtGoWorld)
		{
			uint8_t selectedch = static_cast<uint8_t>(id - Buttons::BtChannel0);

			if (selectedch != channelid)
			{
				buttons[Buttons::BtChannel0 + channelid]->set_state(Button::State::NORMAL);
				channelid = static_cast<uint8_t>(id - Buttons::BtChannel0);
				buttons[Buttons::BtChannel0 + channelid]->set_state(Button::State::PRESSED);
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
		LOG(LOG_DEBUG, "[UIWorldSelect] Setting world ID: " + std::to_string(worldid) + " channel ID: " + std::to_string(channelid));
		
		Configuration::get().set_worldid(worldid);
		Configuration::get().set_channelid(channelid);

		LOG(LOG_DEBUG, "[UIWorldSelect] Creating UILoginWait");
		UI::get().emplace<UILoginWait>();
		auto loginwait = UI::get().get_element<UILoginWait>();

		if (loginwait && loginwait->is_active())
		{
			LOG(LOG_DEBUG, "[UIWorldSelect] Sending CharlistRequestPacket");
			CharlistRequestPacket(worldid, channelid).dispatch();
		}
		else
		{
			LOG(LOG_ERROR, "[UIWorldSelect] UILoginWait not active or not found");
		}
	}

	void UIWorldSelect::clear_selected_world()
	{
		channelid = 0;

		// Only clear channel buttons if they exist (v83 compatibility)
		for (size_t i = Buttons::BtChannel0; i < Buttons::BtGoWorld; i++)
		{
			if (buttons[i])
				buttons[i]->set_state(Button::State::NORMAL);
		}

		if (buttons[Buttons::BtChannel0])
			buttons[Buttons::BtChannel0]->set_state(Button::State::PRESSED);

		for (size_t i = 0; i < Buttons::BtGoWorld - Buttons::BtChannel0; i++)
		{
			if (buttons[Buttons::BtChannel0 + i])
			{
				buttons[Buttons::BtChannel0 + i]->set_active(false);
				channel_gauge[i].update(0);
			}
		}

		if (buttons[Buttons::BtGoWorld])
			buttons[Buttons::BtGoWorld]->set_active(false);

		worldNoticeMessage.change_text("");
	}

	uint16_t UIWorldSelect::get_next_world(uint16_t id, bool upward)
	{
		uint16_t next_world;

		if (world_map[Buttons::BtWorld0] == Worlds::SCANIA)
		{
			switch (id)
			{
				case Buttons::BtWorld0:
					next_world = (upward) ? Worlds::REBOOT0 : Worlds::BERA;
					break;
				case Buttons::BtWorld1:
					next_world = (upward) ? Worlds::SCANIA : Worlds::AURORA;
					break;
				case Buttons::BtWorld2:
					next_world = (upward) ? Worlds::BERA : Worlds::ELYSIUM;
					break;
				case Buttons::BtWorld3:
					next_world = (upward) ? Worlds::AURORA : Worlds::REBOOT0;
					break;
				case Buttons::BtWorld4:
					next_world = (upward) ? Worlds::ELYSIUM : Worlds::SCANIA;
					break;
				default:
					break;
			}
		}
		else
		{
			switch (id)
			{
				case Buttons::BtWorld0:
					next_world = (upward) ? Worlds::REBOOT0 : Worlds::REBOOT0;
					break;
				case Buttons::BtWorld4:
					next_world = (upward) ? Worlds::SCANIA : Worlds::SCANIA;
					break;
				default:
					break;
			}
		}

		auto world = world_map.begin();

		while (world != world_map.end())
		{
			if (world->second == next_world)
				return world->first;

			world++;
		}

		return Worlds::SCANIA;
	}
}