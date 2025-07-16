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
#include "ItemParser.h"
#include "../../../Util/Misc.h"

namespace ms
{
	namespace ItemParser
	{
		// Parse a normal item from a packet
		void add_item(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
		{
			LOG(LOG_DEBUG, "[ItemParser] add_item starting for ID " << id << " at slot " << slot);
			
			// TEMP FIX: Handle problematic items that cause hangs in v83/v87
			if (id == 23691776 || id > 20000000) {
				LOG(LOG_ERROR, "[ItemParser] Problematic item ID " << id << " detected, using safe parsing");
				// Still need to read the data to keep packet aligned, but handle carefully
				bool cash = recv.read_bool();
				if (cash) recv.skip(8);
				recv.read_long(); // expire
				recv.read_short(); // count
				
				// Try to read owner string safely - manual approach for problematic items
				int16_t string_length = recv.read_short();
				if (string_length > 0 && string_length < 100) {
					recv.skip(string_length); // Skip the string bytes
				} else {
					LOG(LOG_ERROR, "[ItemParser] Invalid string length " << string_length << " for problematic item, skipping");
					return;
				}
				
				recv.read_short(); // flag
				
				// Skip adding to inventory for problematic items
				LOG(LOG_DEBUG, "[ItemParser] Skipped adding problematic item to inventory");
				return;
			}
			
			// Read all item stats
			bool cash = recv.read_bool();
			LOG(LOG_DEBUG, "[ItemParser] Cash flag: " << cash);

			if (cash)
				recv.skip(8); // unique id

			int64_t expire = recv.read_long();
			LOG(LOG_DEBUG, "[ItemParser] Expire: " << expire);
			
			int16_t count = recv.read_short();
			LOG(LOG_DEBUG, "[ItemParser] Count: " << count);
			
			std::string owner;
			try {
				owner = recv.read_string();
				LOG(LOG_DEBUG, "[ItemParser] Owner: " << owner);
			} catch (...) {
				LOG(LOG_ERROR, "[ItemParser] Failed to read owner string, using empty string");
				owner = "";
			}
			
			int16_t flag = recv.read_short();
			LOG(LOG_DEBUG, "[ItemParser] Flag: " << flag);

			// If the item is a rechargeable projectile, some additional bytes are sent.
			if ((id / 10000 == 233) || (id / 10000 == 207))
				recv.skip(8);

			LOG(LOG_DEBUG, "[ItemParser] Adding item to inventory");
			inventory.add_item(invtype, slot, id, cash, expire, count, owner, flag);
			LOG(LOG_DEBUG, "[ItemParser] add_item completed for ID " << id);
		}

		// Parse a pet from a packet
		void add_pet(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
		{
			// Read all pet stats
			bool cash = recv.read_bool();

			if (cash)
				recv.skip(8); // unique id

			int64_t expire = recv.read_long();
			std::string petname = recv.read_padded_string(13);
			int8_t petlevel = recv.read_byte();
			int16_t closeness = recv.read_short();
			int8_t fullness = recv.read_byte();

			// Some unused bytes
			recv.skip(18);

			inventory.add_pet(invtype, slot, id, cash, expire, petname, petlevel, closeness, fullness);
		}

		// Parse an equip from a packet
		void add_equip(InPacket& recv, InventoryType::Id invtype, int16_t slot, int32_t id, Inventory& inventory)
		{
			// Read equip information
			bool cash = recv.read_bool();

			if (cash)
				recv.skip(8); // unique id

			int64_t expire = recv.read_long();
			uint8_t slots = recv.read_byte();
			uint8_t level = recv.read_byte();

			// Read equip stats
			EnumMap<EquipStat::Id, uint16_t> stats;

			for (auto iter : stats)
				iter.second = recv.read_short();

			// Some more information
			std::string owner = recv.read_string();
			int16_t flag = recv.read_short();
			uint8_t itemlevel = 0;
			uint16_t itemexp = 0;
			int32_t vicious = 0;

			if (cash)
			{
				// Some unused bytes
				recv.skip(10);
			}
			else
			{
				recv.read_byte();
				itemlevel = recv.read_byte();
				recv.read_short();
				itemexp = recv.read_short();
				vicious = recv.read_int();
				recv.read_long();
			}

			recv.skip(12);

			if (slot < 0)
			{
				invtype = InventoryType::Id::EQUIPPED;
				slot = -slot;
			}

			inventory.add_equip(invtype, slot, id, cash, expire, slots, level, stats, owner, flag, itemlevel, itemexp, vicious);
		}

		void parse_item(InPacket& recv, InventoryType::Id invtype, int16_t slot, Inventory& inventory)
		{
			LOG(LOG_DEBUG, "[ItemParser] Starting parse_item for slot " << slot);
			
			// Read type and item id
			recv.read_byte(); // 'type' byte
			int32_t iid = recv.read_int();
			LOG(LOG_DEBUG, "[ItemParser] Item ID: " << iid << " for slot " << slot);

			if (invtype == InventoryType::Id::EQUIP || invtype == InventoryType::Id::EQUIPPED)
			{
				// Parse an equip
				LOG(LOG_DEBUG, "[ItemParser] Parsing equip for slot " << slot);
				add_equip(recv, invtype, slot, iid, inventory);
				LOG(LOG_DEBUG, "[ItemParser] Finished parsing equip for slot " << slot);
			}
			else if (iid >= 5000000 && iid <= 5000102)
			{
				// Parse a pet
				LOG(LOG_DEBUG, "[ItemParser] Parsing pet for slot " << slot);
				add_pet(recv, invtype, slot, iid, inventory);
				LOG(LOG_DEBUG, "[ItemParser] Finished parsing pet for slot " << slot);
			}
			else
			{
				// Parse a normal item
				LOG(LOG_DEBUG, "[ItemParser] Parsing normal item for slot " << slot);
				add_item(recv, invtype, slot, iid, inventory);
				LOG(LOG_DEBUG, "[ItemParser] Finished parsing normal item for slot " << slot);
			}
			
			LOG(LOG_DEBUG, "[ItemParser] Completed parse_item for slot " << slot);
		}
	}
}