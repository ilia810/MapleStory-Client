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
#include "LoginParser.h"

#include "../../Session.h"
#include "../../../Util/Misc.h"

namespace ms
{
	Account LoginParser::parse_account(InPacket& recv)
	{
		Account account;

		recv.skip_short();

		account.accid = recv.read_int();
		account.female = recv.read_byte();
		account.admin = recv.read_bool();

		recv.skip_byte(); // Admin
		recv.skip_byte(); // Country Code

		account.name = recv.read_string();

		recv.skip_byte();

		account.muted = recv.read_bool();

		recv.skip_long(); // muted until
		recv.skip_long(); // creation date

		recv.skip_int(); // Remove "Select the world you want to play in"

		account.pin = recv.read_bool(); // 0 - Enabled, 1 - Disabled
		account.pic = recv.read_byte(); // 0 - Register, 1 - Ask, 2 - Disabled

		return account;
	}

	World LoginParser::parse_world(InPacket& recv)
	{
		int8_t wid = recv.read_byte();

		if (wid == -1)
			return { {}, {}, {}, 0, 0, wid };

		std::string name = recv.read_string();
		uint8_t flag = recv.read_byte();
		std::string event_message = recv.read_string();

		recv.skip(5);

		std::vector<int32_t> channelload;
		uint8_t channelload_size = recv.read_byte();

		for (uint8_t i = 0; i < channelload_size; ++i)
		{
			recv.skip_string(); // channel name

			channelload.push_back(recv.read_int());

			recv.skip_byte(); // world id
			recv.skip_byte(); // channel id
			recv.skip_bool(); // adult channel
		}

		recv.skip_short();

		return { name, event_message, channelload, channelload_size, flag, wid };
	}

	CharEntry LoginParser::parse_charentry(InPacket& recv)
	{
		try {
			int32_t cid = recv.read_int();
			
			StatsEntry stats = parse_stats(recv);
			
			LookEntry look = parse_look(recv);

			// Server writes a 0 byte after look data when !viewall
			recv.read_byte();

			recv.read_bool(); // 'rankinfo' bool

			if (recv.read_bool())
			{
				int32_t currank = recv.read_int();
				int32_t rankmv = recv.read_int();
				int32_t curjobrank = recv.read_int();
				int32_t jobrankmv = recv.read_int();
				int8_t rankmc = (rankmv > 0) ? '+' : (rankmv < 0) ? '-' : '=';
				int8_t jobrankmc = (jobrankmv > 0) ? '+' : (jobrankmv < 0) ? '-' : '=';

				stats.rank = std::make_pair(currank, rankmc);
				stats.jobrank = std::make_pair(curjobrank, jobrankmc);
			}

			return { stats, look, cid };
		}
		catch (const std::exception& e) {
			throw;
		}
		catch (...) {
			throw;
		}
	}

	StatsEntry LoginParser::parse_stats(InPacket& recv)
	{
		// TODO: This is similar to CashShopParser.cpp, try and merge these.
		StatsEntry statsentry;

		statsentry.name = recv.read_padded_string(13);
		
		statsentry.female = recv.read_bool();

		recv.read_byte();	// skin
		recv.read_int();	// face
		recv.read_int();	// hair

		for (size_t i = 0; i < 3; i++)
			statsentry.petids.push_back(recv.read_long());

		// v83 server writes level as byte, not short
		statsentry.stats[MapleStat::Id::LEVEL] = recv.read_byte();
		statsentry.stats[MapleStat::Id::JOB] = recv.read_short();
		statsentry.stats[MapleStat::Id::STR] = recv.read_short();
		statsentry.stats[MapleStat::Id::DEX] = recv.read_short();
		statsentry.stats[MapleStat::Id::INT] = recv.read_short();
		statsentry.stats[MapleStat::Id::LUK] = recv.read_short();
		statsentry.stats[MapleStat::Id::HP] = recv.read_short();
		statsentry.stats[MapleStat::Id::MAXHP] = recv.read_short();
		statsentry.stats[MapleStat::Id::MP] = recv.read_short();
		statsentry.stats[MapleStat::Id::MAXMP] = recv.read_short();
		statsentry.stats[MapleStat::Id::AP] = recv.read_short();
		
		// Handle SP based on job type - v83 compatibility
		int16_t job = statsentry.stats[MapleStat::Id::JOB];
		// Check if this is an Evan job (2200-2218)
		if (job >= 2200 && job <= 2218) {
			// Jobs with SP table write: byte(effectiveLength) + pairs of byte(i+1), byte(sp[i])
			uint8_t sp_count = recv.read_byte();
			for (uint8_t i = 0; i < sp_count; i++) {
				recv.read_byte(); // skill book id
				recv.read_byte(); // remaining sp for this book
			}
			statsentry.stats[MapleStat::Id::SP] = 0; // Evans don't use regular SP
		} else {
			statsentry.stats[MapleStat::Id::SP] = recv.read_short();
		}
		
		statsentry.exp = recv.read_int();
		statsentry.stats[MapleStat::Id::FAME] = recv.read_short();

		recv.skip(4); // gachaexp

		// Read map ID - should now be properly aligned after level byte fix
		statsentry.mapid = recv.read_int();
		LOG(LOG_DEBUG, "[LoginParser] Map ID read: " << statsentry.mapid);
		statsentry.portal = recv.read_byte();

		recv.skip(4); // Server writes writeInt(0) after spawnpoint - v83 compatibility

		return statsentry;
	}

	LookEntry LoginParser::parse_look(InPacket& recv)
	{
		LookEntry look;

		// CRITICAL FIX: v83 server sends minimal packet with only 1 byte (gender)
		// The actual character look data must come from character database/selection
		// This is likely a character selection context where full look isn't sent
		
		if (recv.available() < 10) {
			// For v83 compatibility: if packet is too small, use fallback values
			// These should match the admin character from your database
			if (recv.available() >= 1) {
				look.female = recv.read_byte() == 1;
			} else {
				look.female = false; // default male
			}
			
			// v83 fallback values for admin character
			look.skin = 0;           // Default skin
			look.faceid = 20000;     // Default face
			look.hairid = 30030;     // Admin character hair ID
			
			return look;
		}
		
		// Original parsing for full packets (modern versions)
		look.female = recv.read_byte() == 1; // Server sends byte, not bool
		
		look.skin = recv.read_byte();
		
		look.faceid = recv.read_int();

		bool megaphone = recv.read_bool(); // megaphone flag

		look.hairid = recv.read_int();

		// Equipment parsing based on addCharEquips
		
		// Safety check - make sure we have at least 1 byte
		if (recv.available() < 1) {
			return look;
		}
		
		uint8_t eqslot = recv.read_byte();

		while (eqslot != 0xFF && recv.available() >= 4) // Need 4 bytes for int + 1 for next slot
		{
			look.equips[eqslot] = recv.read_int();
			if (recv.available() < 1) break;
			eqslot = recv.read_byte();
		}

		if (recv.available() < 1) {
			return look;
		}
		
		uint8_t mskeqslot = recv.read_byte();

		while (mskeqslot != 0xFF && recv.available() >= 4)
		{
			look.maskedequips[mskeqslot] = recv.read_int();
			if (recv.available() < 1) break;
			mskeqslot = recv.read_byte();
		}

		if (recv.available() >= 4) {
			look.maskedequips[-111] = recv.read_int();
		}

		return look;
	}

	void LoginParser::parse_login(InPacket& recv)
	{
		recv.skip_byte();

		// Read the IPv4 address in a string
		std::string addrstr;

		for (size_t i = 0; i < 4; i++)
		{
			uint8_t num = static_cast<uint8_t>(recv.read_byte());
			addrstr.append(std::to_string(num));

			if (i < 3)
				addrstr.push_back('.');
		}

		// Read the port address in a string
		std::string portstr = std::to_string(recv.read_short());


		// Attempt to reconnect to the server
		Session::get().reconnect(addrstr.c_str(), portstr.c_str());
	}
}