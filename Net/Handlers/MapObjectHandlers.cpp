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
#include "MapObjectHandlers.h"

#include "Helpers/LoginParser.h"
#include "Helpers/MovementParser.h"

#include "../../Gameplay/Stage.h"

#include <iomanip>
#include <vector>
#include <iostream>

namespace ms
{
	// Temporary storage for NPCs received during map transitions
	struct PendingNpc {
		int32_t oid;
		int32_t id;
		Point<int16_t> position;
		bool flip;
		uint16_t fh;
		int32_t mapid;
	};
	static std::vector<PendingNpc> pending_npcs;
	void SpawnCharHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();

		// We don't need to spawn the player twice
		if (Stage::get().is_player(cid))
			return;

		uint16_t level = recv.read_short();
		std::string name = recv.read_string();

		recv.read_string();	// guildname
		recv.read_short();	// guildlogobg
		recv.read_byte();	// guildlogobgcolor
		recv.read_short();	// guildlogo
		recv.read_byte();	// guildlogocolor

		recv.skip(8);

		bool morphed = recv.read_int() == 2;
		int32_t buffmask1 = recv.read_int();
		int16_t buffvalue = 0;

		if (buffmask1 != 0)
			buffvalue = morphed ? recv.read_short() : recv.read_byte();

		recv.read_int(); // buffmask 2

		recv.skip(43);

		recv.read_int(); // 'mount'

		recv.skip(61);

		int16_t job = recv.read_short();
		LookEntry look = LoginParser::parse_look(recv);

		recv.read_int(); // count of 5110000 
		recv.read_int(); // 'itemeffect'
		recv.read_int(); // 'chair'

		Point<int16_t> position = recv.read_point();
		int8_t stance = recv.read_byte();

		recv.skip(3);

		for (size_t i = 0; i < 3; i++)
		{
			int8_t available = recv.read_byte();

			if (available == 1)
			{
				recv.read_byte();	// 'byte2'
				recv.read_int();	// petid
				recv.read_string();	// name
				recv.read_int();	// unique id
				recv.read_int();
				recv.read_point();	// pos
				recv.read_byte();	// stance
				recv.read_int();	// fhid
			}
			else
			{
				break;
			}
		}

		recv.read_int(); // mountlevel
		recv.read_int(); // mountexp
		recv.read_int(); // mounttiredness

		// TODO: Shop stuff
		recv.read_byte();
		// TODO: Shop stuff end

		bool chalkboard = recv.read_bool();
		std::string chalktext = chalkboard ? recv.read_string() : "";

		recv.skip(3);
		recv.read_byte(); // team

		Stage::get().get_chars().spawn(
			{ cid, look, level, job, name, stance, position }
		);
	}

	void RemoveCharHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();

		Stage::get().get_chars().remove(cid);
	}

	void SpawnPetHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		Optional<Char> character = Stage::get().get_character(cid);

		if (!character)
			return;

		uint8_t petindex = recv.read_byte();
		int8_t mode = recv.read_byte();

		if (mode == 1)
		{
			recv.skip(1);

			int32_t itemid = recv.read_int();
			std::string name = recv.read_string();
			int32_t uniqueid = recv.read_int();

			recv.skip(4);

			Point<int16_t> pos = recv.read_point();
			uint8_t stance = recv.read_byte();
			int32_t fhid = recv.read_int();

			character->add_pet(petindex, itemid, name, uniqueid, pos, stance, fhid);
		}
		else if (mode == 0)
		{
			bool hunger = recv.read_bool();

			character->remove_pet(petindex, hunger);
		}
	}

	void CharMovedHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.skip(4);
		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		Stage::get().get_chars().send_movement(cid, movements);
	}

	void UpdateCharLookHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		recv.read_byte();
		LookEntry look = LoginParser::parse_look(recv);

		Stage::get().get_chars().update_look(cid, look);
	}

	void ShowForeignEffectHandler::handle(InPacket& recv) const
	{
		int32_t cid = recv.read_int();
		int8_t effect = recv.read_byte();

		if (effect == 10) // recovery
		{
			recv.read_byte(); // 'amount'
		}
		else if (effect == 13) // card effect
		{
			Stage::get().show_character_effect(cid, CharEffect::MONSTER_CARD);
		}
		else if (recv.available()) // skill
		{
			int32_t skillid = recv.read_int();
			recv.read_byte(); // 'direction'
			// 9 more bytes after this

			Stage::get().get_combat().show_buff(cid, skillid, effect);
		}
		else
		{
			// TODO: Blank
		}
	}

	void SpawnMobHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		recv.read_byte(); // 5 if controller == null
		int32_t id = recv.read_int();

		// CRITICAL FIX: The v83 spawn mob packet structure is:
		// After mob ID, when requestController is false:
		// - 16 bytes: skip (as seen in server code: p.skip(16))
		// Then:
		// - Position (4 bytes)
		// - Stance (1 byte)
		// - etc.
		
		// The server code shows: if not requestController, p.skip(16)
		// This matches what we need to skip before position
		recv.skip(16);

		Point<int16_t> position = recv.read_point();
		int8_t stance = recv.read_byte();
		recv.skip(2);
		uint16_t fh = recv.read_short();
		
		// Check if we have enough bytes for the full packet format
		if (recv.available() < 5) {
			// Simplified packet format - no effect/team data
			Stage::get().get_mobs().spawn(
				{ oid, id, 1, stance, fh, false, -1, position }
			);
			return;
		}
		
		int8_t effect = recv.read_byte();

		if (effect > 0)
		{
			recv.read_byte();
			recv.read_short();

			if (effect == 15)
				recv.read_byte();
		}

		int8_t team = recv.read_byte();

		recv.skip(4);

		Stage::get().get_mobs().spawn(
			{ oid, id, 1, stance, fh, effect == -2, team, position }
		);
	}

	void KillMobHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t animation = recv.read_byte();

		Stage::get().get_mobs().remove(oid, animation);
	}

	void SpawnMobControllerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();
		
		std::cout << "[DEBUG] SpawnMobController: oid=" << oid << ", mode=" << (int)mode << std::endl;

		if (mode == 0)
		{
			Stage::get().get_mobs().set_control(oid, false);
		}
		else
		{
			if (recv.available())
			{
				recv.skip(1);

				int32_t id = recv.read_int();

				// CRITICAL FIX: For controller packets, we need to parse the actual status data
				// The structure after mob ID includes the full encodeTemporary data
				// which is variable length. For now, let's skip a fixed amount that works
				// for most common cases
				recv.skip(22); // This works for monsters with no active status effects

				Point<int16_t> position = recv.read_point();
				int8_t stance = recv.read_byte();

				recv.skip(2);

				uint16_t fh = recv.read_short();
				int8_t effect = recv.read_byte();

				if (effect > 0)
				{
					recv.read_byte();
					recv.read_short();

					if (effect == 15)
						recv.read_byte();
				}

				int8_t team = recv.read_byte();

				recv.skip(4);

				Stage::get().get_mobs().spawn(
					{ oid, id, mode, stance, fh, effect == -2, team, position }
				);
			}
			else
			{
				// TODO: Remove monster invisibility, not used (maybe in an event script?), Check this!
			}
		}
	}

	void MobMovedHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();

		int8_t byte1 = recv.read_byte(); // pNibbles
		int8_t rawActivity = recv.read_byte();
		int8_t skill_id = recv.read_byte();
		int8_t skill_level = recv.read_byte();
		int16_t pOption = recv.read_short();
		recv.skip(8); // Skip 8 bytes as server does

		// Check if this is actually a skill usage
		bool isSkill = (rawActivity >= 42 && rawActivity <= 59);
		
		// Debug all received values
		std::cout << "[DEBUG] MobMoved packet - oid: " << oid 
				  << ", byte1: " << (int)byte1 
				  << ", rawActivity: " << (int)rawActivity 
				  << ", skill_id: " << (int)skill_id 
				  << ", skill_level: " << (int)skill_level 
				  << ", pOption: " << pOption << std::endl;
		
		recv.read_byte(); // Skip one more byte
		recv.read_int(); // Skip 4 bytes

		Point<int16_t> position = recv.read_point();
		std::vector<Movement> movements = MovementParser::parse_movements(recv);

		// Check if skill_id and skill_level are set (server is telling us mob used a skill)
		if (skill_id > 0 && skill_level > 0)
		{
			std::cout << "[DEBUG] Mob " << oid << " ACTUALLY using skill " << (int)skill_id << " level " << (int)skill_level << std::endl;
			Stage::get().get_mobs().send_skill(oid, skill_id, skill_level);
		}
		else if (rawActivity >= 42)  // Force skill animation based on rawActivity
		{
			std::cout << "[DEBUG] Mob " << oid << " forcing skill animation based on rawActivity=" << (int)rawActivity << std::endl;
			// Use a default skill animation
			Stage::get().get_mobs().send_skill(oid, 1, 1);
		}
		else if (isSkill)
		{
			std::cout << "[DEBUG] Mob " << oid << " in skill stance but no skill data (rawActivity: " << (int)rawActivity << ")" << std::endl;
		}
		else if (rawActivity >= 24 && rawActivity <= 41)
		{
			// Regular attack animation
			int attackIndex = (rawActivity - 24) / 2;
			std::cout << "[DEBUG] Mob " << oid << " using attack " << attackIndex << " (rawActivity: " << (int)rawActivity << ")" << std::endl;
			Stage::get().get_mobs().send_skill(oid, attackIndex + 1, 1); // Use attack animations for regular attacks
		}

		Stage::get().get_mobs().send_movement(oid, position, std::move(movements));
	}

	void ShowMobHpHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t hppercent = recv.read_byte();
		uint16_t playerlevel = Stage::get().get_player().get_stats().get_stat(MapleStat::Id::LEVEL);

		Stage::get().get_mobs().send_mobhp(oid, hppercent, playerlevel);
	}

	void ApplyMonsterStatusHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		recv.read_long(); // 0
		
		// Read the status mask (4 bytes in v83 for each status)
		int32_t mask1 = recv.read_int();
		int32_t mask2 = recv.read_int();
		int32_t mask3 = recv.read_int();
		int32_t mask4 = recv.read_int();
		
		// For now, we'll trigger attack animation when any status is applied
		// This gives visual feedback that the mob is doing something
		if (mask1 != 0 || mask2 != 0 || mask3 != 0 || mask4 != 0)
		{
			// Trigger a skill animation
			// We use a generic skill ID since we don't have the actual skill info here
			Stage::get().get_mobs().send_skill(oid, 1, 1);
		}
		
		// TODO: Parse the actual status effects and durations
		// The packet contains status values and skill IDs for each active status
	}

	void SpawnNpcHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int32_t id = recv.read_int();
		Point<int16_t> position = recv.read_point();
		bool flip = recv.read_bool();
		uint16_t fh = recv.read_short();

		recv.read_short(); // 'rx'
		recv.read_short(); // 'ry'
		recv.read_byte();  // Extra byte sent by v83 server

		Stage::get().get_npcs().spawn(
			{ oid, id, position, flip, fh }
		);
	}

	void SpawnNpcControllerHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();

		if (mode == 0)
		{
			Stage::get().get_npcs().remove(oid);
		}
		else
		{
			int32_t id = recv.read_int();
			Point<int16_t> position = recv.read_point();
			bool flip = recv.read_bool();
			uint16_t fh = recv.read_short();

			recv.read_short();	// 'rx'
			recv.read_short();	// 'ry'
			recv.read_bool();	// 'minimap'

			Stage::get().get_npcs().spawn(
				{ oid, id, position, flip, fh }
			);
		}
	}

	void DropLootHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();
		bool meso = recv.read_bool();
		int32_t itemid = recv.read_int();
		int32_t owner = recv.read_int();
		int8_t pickuptype = recv.read_byte();
		Point<int16_t> dropto = recv.read_point();

		recv.skip(4);

		Point<int16_t> dropfrom;

		if (mode != 2)
		{
			dropfrom = recv.read_point();
			recv.skip(2);

			Sound(Sound::Name::DROP).play();
		}
		else
		{
			dropfrom = dropto;
		}

		if (!meso)
			recv.skip(8);

		bool playerdrop = !recv.read_bool();

		Stage::get().get_drops().spawn(
			{ oid, itemid, meso, owner, dropfrom, dropto, pickuptype, mode, playerdrop }
		);
	}

	void RemoveLootHandler::handle(InPacket& recv) const
	{
		int8_t mode = recv.read_byte();
		int32_t oid = recv.read_int();

		Optional<PhysicsObject> looter;

		if (mode > 1)
		{
			int32_t cid = recv.read_int();

			if (recv.length() > 0)
				recv.read_byte(); // pet
			else if (auto character = Stage::get().get_character(cid))
				looter = character->get_phobj();

			Sound(Sound::Name::PICKUP).play();
		}

		Stage::get().get_drops().remove(oid, mode, looter.get());
	}

	void HitReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();
		int8_t stance = recv.read_byte(); // TODO: When is this different than state?
		recv.skip(2); // TODO: Unused
		recv.skip(1); // "frame" delay but this is in the WZ file?

		Stage::get().get_reactors().trigger(oid, state);
	}

	void SpawnReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int32_t rid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();

		// TODO: Unused, Check this!
		// uint16_t fhid = recv.read_short();
		// recv.read_byte()

		Stage::get().get_reactors().spawn(
			{ oid, rid, state, point }
		);
	}

	void RemoveReactorHandler::handle(InPacket& recv) const
	{
		int32_t oid = recv.read_int();
		int8_t state = recv.read_byte();
		Point<int16_t> point = recv.read_point();

		Stage::get().get_reactors().remove(oid, state, point);
	}
}