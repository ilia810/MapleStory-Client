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

#include "../UIElement.h"

#include "../Components/Gauge.h"

#include "../../Net/Login.h"

namespace ms
{
	class UIWorldSelect : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::WORLDSELECT;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIWorldSelect();

		void draw(float alpha) const override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void draw_world();
		void add_world(World world);
		void change_world(World selectedWorld);
		void remove_selected();
		void set_region(uint8_t value);
		uint16_t get_worldbyid(uint16_t worldid);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		static constexpr uint8_t COLUMNS = 6U;
		static constexpr uint8_t FLAG_SIZE = 3U;
		static constexpr int32_t CHANNEL_LOAD = 100;

		void enter_world();
		void clear_selected_world();

		enum Buttons : uint16_t
		{
			BtWorld0,
			BtWorld1,
			BtWorld2,
			BtWorld3,
			BtWorld4,
			BtWorld5,
			BtWorld6,
			BtWorld7,
			BtWorld8,
			BtWorld9,
			BtWorld10,
			BtWorld11,
			BtWorld12,
			BtWorld13,
			BtWorld14,
			BtWorld15,
			BtChannel0 = 20U,
			BtGoWorld = 40U,
			BtViewAll,
			BtExit
		};

		enum Worlds : uint16_t
		{
			SCANIA,
			BERA,
			BROA,
			WINDIA,
			KHAINI,
			BELLOCAN,
			MARDIA,
			KRADIA,
			YELLONDE,
			DEMETHOS,
			GALICIA,
			ELNIDO,
			ZENITH,
			ARCANIA,
			CHAOS,
			NOVA,
			RENEGADES,
			AURORA,
			ELYSIUM,
			KOREAN_ENOSIS = 29,
			LUNA,
			ELYSIUM_LINK,
			LAB = 40,
			KOREAN_RED = 43,
			KOREAN_AURORA,
			REBOOT0,
			REBOOT1,
			PINKBEAN = 48,
			BURNING,
			KOREAN_ARCANE,
			KOREAN_NOVA,
			TESPIA = 100
		};

		// Frame (stretched to 808x608)
		Texture frame;
		Point<int16_t> frame_stretch;

		// Background panel (601x241)
		Texture chBackgrn;
		Point<int16_t> chBackgrn_pos;
		Point<int16_t> chBackgrn_stretch;

		// Version text
		Text version;
		Point<int16_t> version_pos;

		// Channel selection
		Texture channels_background;
		Point<int16_t> channelsrc_pos;
		Point<int16_t> worldsrc_pos;

		// World data
		uint8_t worldid;
		uint8_t channelid;
		uint8_t worldcount;

		std::vector<World> worlds;
		std::vector<Texture> world_textures;

		// World notice
		std::vector<Sprite> flag_sprites;
		Texture worldNotice;
		Texture rebootNotice;
		Text worldNoticeMessage;

		// Channel gauges
		Gauge channel_gauge[BtGoWorld - BtChannel0];

		bool world_selected;

		// NX nodes for dynamic button creation
		nl::node BtWorld;
		nl::node channelsrc;
	};
}
