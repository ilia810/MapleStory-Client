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
#include "CharacterParser.h"

#include "ItemParser.h"
#include "../../../Util/Misc.h"

namespace ms
{
	void CharacterParser::parse_inventory(InPacket& recv, Inventory& invent)
	{
		LOG(LOG_DEBUG, "[CharacterParser] Starting inventory parsing");
		
		invent.set_meso(recv.read_int());
		LOG(LOG_DEBUG, "[CharacterParser] Meso set");
		
		invent.set_slotmax(InventoryType::EQUIP, recv.read_byte());
		invent.set_slotmax(InventoryType::USE, recv.read_byte());
		invent.set_slotmax(InventoryType::SETUP, recv.read_byte());
		invent.set_slotmax(InventoryType::ETC, recv.read_byte());
		invent.set_slotmax(InventoryType::CASH, recv.read_byte());
		LOG(LOG_DEBUG, "[CharacterParser] Slot maxes set");

		recv.skip(8);

		for (size_t i = 0; i < 3; i++)
		{
			InventoryType::Id inv = (i == 0) ? InventoryType::EQUIPPED : InventoryType::EQUIP;
			int16_t pos = recv.read_short();
			LOG(LOG_DEBUG, "[CharacterParser] Reading inventory section " << i << ", first pos: " << pos);

			int safety_counter = 0;
			const int MAX_ITEMS_PER_SECTION = 500; // Safety limit
			
			while (pos != 0 && safety_counter < MAX_ITEMS_PER_SECTION)
			{
				int16_t slot = (i == 1) ? -pos : pos;
				LOG(LOG_DEBUG, "[CharacterParser] Parsing item at slot " << slot);
				ItemParser::parse_item(recv, inv, slot, invent);
				pos = recv.read_short();
				safety_counter++;
			}
			
			if (safety_counter >= MAX_ITEMS_PER_SECTION) {
				LOG(LOG_ERROR, "[CharacterParser] Hit safety limit in inventory section " << i << ", breaking loop");
				break;
			}
			
			LOG(LOG_DEBUG, "[CharacterParser] Finished inventory section " << i << " with " << safety_counter << " items");
		}

		recv.skip(2);

		InventoryType::Id toparse[4] =
		{
			InventoryType::USE, InventoryType::SETUP, InventoryType::ETC, InventoryType::CASH
		};

		for (size_t i = 0; i < 4; i++)
		{
			InventoryType::Id inv = toparse[i];
			int8_t pos = recv.read_byte();
			LOG(LOG_DEBUG, "[CharacterParser] Reading inventory type " << i << ", first pos: " << (int)pos);

			int safety_counter = 0;
			const int MAX_ITEMS_PER_TYPE = 200; // Safety limit
			
			while (pos != 0 && safety_counter < MAX_ITEMS_PER_TYPE)
			{
				// Check for obviously corrupted position values
				if (pos < 0 || pos > 100) {
					LOG(LOG_ERROR, "[CharacterParser] Invalid position " << (int)pos << " detected, breaking inventory parsing");
					break;
				}
				
				LOG(LOG_DEBUG, "[CharacterParser] Parsing item at pos " << (int)pos);
				try {
					ItemParser::parse_item(recv, inv, pos, invent);
				} catch (...) {
					LOG(LOG_ERROR, "[CharacterParser] Failed to parse item at pos " << (int)pos << ", breaking inventory parsing");
					break;
				}
				
				pos = recv.read_byte();
				safety_counter++;
			}
			
			if (safety_counter >= MAX_ITEMS_PER_TYPE) {
				LOG(LOG_ERROR, "[CharacterParser] Hit safety limit in inventory type " << i << ", breaking loop");
				break;
			}
			
			LOG(LOG_DEBUG, "[CharacterParser] Finished inventory type " << i << " with " << safety_counter << " items");
		}
		
		LOG(LOG_DEBUG, "[CharacterParser] Inventory parsing completed");
	}

	void CharacterParser::parse_skillbook(InPacket& recv, SkillBook& skills)
	{
		int16_t size = recv.read_short();

		for (int16_t i = 0; i < size; i++)
		{
			int32_t skill_id = recv.read_int();
			int32_t level = recv.read_int();
			int64_t expiration = recv.read_long();
			bool fourthtjob = ((skill_id % 100000) / 10000 == 2);
			int32_t masterlevel = fourthtjob ? recv.read_int() : 0;
			skills.set_skill(skill_id, level, masterlevel, expiration);
		}
	}

	void CharacterParser::parse_cooldowns(InPacket& recv, Player& player)
	{
		LOG(LOG_DEBUG, "[CharacterParser] Starting cooldowns parsing");
		int16_t size = recv.read_short();
		LOG(LOG_DEBUG, "[CharacterParser] Cooldowns size: " << size);

		// Add safety check for size
		if (size > 1000 || size < 0) {
			LOG(LOG_ERROR, "[CharacterParser] Invalid cooldowns size " << size << ", skipping");
			return;
		}

		for (int16_t i = 0; i < size; i++)
		{
			LOG(LOG_DEBUG, "[CharacterParser] Parsing cooldown " << i << " of " << size);
			int32_t skill_id = recv.read_int();
			int32_t cooltime = recv.read_short();
			LOG(LOG_DEBUG, "[CharacterParser] Cooldown - skill_id: " << skill_id << ", cooltime: " << cooltime);
			player.add_cooldown(skill_id, cooltime);
		}
		LOG(LOG_DEBUG, "[CharacterParser] Cooldowns parsing completed");
	}

	void CharacterParser::parse_questlog(InPacket& recv, QuestLog& quests)
	{
		int16_t size = recv.read_short();

		for (int16_t i = 0; i < size; i++)
		{
			int16_t qid = recv.read_short();
			std::string qdata = recv.read_string();

			if (quests.is_started(qid))
			{
				int16_t qidl = quests.get_last_started();
				quests.add_in_progress(qidl, qid, qdata);
				//i--; // This was causing issues
			}
			else
			{
				quests.add_started(qid, qdata);
			}
		}

		std::map<int16_t, int64_t> completed;
		size = recv.read_short();

		for (int16_t i = 0; i < size; i++)
		{
			int16_t qid = recv.read_short();
			int64_t time = recv.read_long();
			quests.add_completed(qid, time);
		}
	}

	void CharacterParser::parse_ring1(InPacket& recv)
	{
		int16_t rsize = recv.read_short();

		for (int16_t i = 0; i < rsize; i++)
		{
			recv.read_int();
			recv.read_padded_string(13);
			recv.read_int();
			recv.read_int();
			recv.read_int();
			recv.read_int();
		}
	}

	void CharacterParser::parse_ring2(InPacket& recv)
	{
		int16_t rsize = recv.read_short();

		for (int16_t i = 0; i < rsize; i++)
		{
			recv.read_int();
			recv.read_padded_string(13);
			recv.read_int();
			recv.read_int();
			recv.read_int();
			recv.read_int();
			recv.read_int();
		}
	}

	void CharacterParser::parse_ring3(InPacket& recv)
	{
		int16_t rsize = recv.read_short();

		for (int16_t i = 0; i < rsize; i++)
		{
			recv.read_int();
			recv.read_int();
			recv.read_int();
			recv.read_short();
			recv.read_int();
			recv.read_int();
			recv.read_padded_string(13);
			recv.read_padded_string(13);
		}
	}

	void CharacterParser::parse_minigame(InPacket& recv)
	{
		recv.skip(2);
	}

	void CharacterParser::parse_monsterbook(InPacket& recv, MonsterBook& monsterbook)
	{
		LOG(LOG_DEBUG, "[CharacterParser] Starting monsterbook parsing");
		LOG(LOG_DEBUG, "[CharacterParser] Remaining bytes before monsterbook: " << recv.length());
		
		int32_t cover = recv.read_int();
		LOG(LOG_DEBUG, "[CharacterParser] Monsterbook cover: " << cover);
		monsterbook.set_cover(cover);

		recv.skip(1);

		int16_t size = recv.read_short();
		LOG(LOG_DEBUG, "[CharacterParser] Monsterbook size: " << size);
		
		// Sanity check - monsterbook shouldn't have more than 1000 entries
		if (size < 0 || size > 1000) {
			LOG(LOG_ERROR, "[CharacterParser] Invalid monsterbook size " << size << ", skipping");
			return;
		}
		
		// Check if we have enough data for all entries
		size_t required_bytes = size * 3;
		if (recv.length() < required_bytes) {
			LOG(LOG_DEBUG, "[CharacterParser] Not enough data for monsterbook (need " << required_bytes << ", have " << recv.length() << "), adjusting size");
			size = static_cast<int16_t>(recv.length() / 3);
		}

		for (int16_t i = 0; i < size; i++)
		{
			if (recv.length() < 3) {
				LOG(LOG_ERROR, "[CharacterParser] Not enough bytes for monsterbook entry " << i << " (need 3, have " << recv.length() << ")");
				break;
			}
			int16_t cid = recv.read_short();
			int8_t mblv = recv.read_byte();

			monsterbook.add_card(cid, mblv);
		}
		
		LOG(LOG_DEBUG, "[CharacterParser] Monsterbook parsing completed");
	}

	void CharacterParser::parse_teleportrock(InPacket& recv, TeleportRock& teleportrock)
	{
		for (size_t i = 0; i < 5; i++)
			teleportrock.addlocation(recv.read_int());

		for (size_t i = 0; i < 10; i++)
			teleportrock.addviplocation(recv.read_int());
	}

	void CharacterParser::parse_nyinfo(InPacket& recv)
	{
		int16_t nysize = recv.read_short();

		for (int16_t i = 0; i < nysize; i++)
		{
			recv.read_int();	// NewYear Id
			recv.read_int();	// NewYear SenderId
			recv.read_string();	// NewYear SenderName
			recv.read_bool();	// NewYear enderCardDiscarded
			recv.read_long();	// NewYear DateSent
			recv.read_int();	// NewYear ReceiverId
			recv.read_string();	// NewYear ReceiverName
			recv.read_bool();	// NewYear eceiverCardDiscarded
			recv.read_bool();	// NewYear eceiverCardReceived
			recv.read_long();	// NewYear DateReceived
			recv.read_string();	// NewYear Message
		}
	}

	void CharacterParser::parse_areainfo(InPacket& recv)
	{
		std::map<int16_t, std::string> areainfo;
		int16_t arsize = recv.read_short();

		for (int16_t i = 0; i < arsize; i++)
		{
			int16_t area = recv.read_short();
			areainfo[area] = recv.read_string();
		}
	}
}