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
#include "SetFieldHandlers.h"

#include "Helpers/CharacterParser.h"
#include "Helpers/LoginParser.h"

#include "../Packets/GameplayPackets.h"

#include "../../Configuration.h"

#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/Window.h"
#include "../../Character/Look/EquipSlot.h"

#include "../../IO/UITypes/UICharSelect.h"
#include "../../Util/Misc.h"

namespace ms
{
	void SetFieldHandler::transition(int32_t mapid, uint8_t portalid) const
	{
		LOG(LOG_DEBUG, "[SetFieldHandler] transition() called - mapid: " << mapid << ", portalid: " << (int)portalid);
		float fadestep = 0.025f;

		// Starting fadeout for mapid and portalid
		LOG(LOG_DEBUG, "[SetFieldHandler] Starting fadeout with fadestep: " << fadestep);
		
		Window::get().fadeout(
			fadestep,
			[mapid, portalid, fadestep]()
			{
				LOG(LOG_DEBUG, "[SetFieldHandler] Fadeout callback started - loading map " << mapid << " portal " << (int)portalid);
				
				// Lock graphics during map loading
				LOG(LOG_DEBUG, "[SetFieldHandler] Locking graphics for map load");
				GraphicsGL::get().lock();
				
				// Clear graphics before loading new map
				LOG(LOG_DEBUG, "[SetFieldHandler] Clearing graphics");
				GraphicsGL::get().clear();
				
				// Only clear and reload if not already in game
				// UIStateGame may have already loaded the Stage
				LOG(LOG_DEBUG, "[SetFieldHandler] Loading map " << mapid << " with portal " << (int)portalid);
				Stage::get().load(mapid, portalid);
				LOG(LOG_DEBUG, "[SetFieldHandler] Map loaded successfully");

				// Calling UI::enable()
				LOG(LOG_DEBUG, "[SetFieldHandler] Enabling UI");
				UI::get().enable();
				
				// Calling Timer::start()
				LOG(LOG_DEBUG, "[SetFieldHandler] Starting timer");
				Timer::get().start();

				// Calling Stage::transfer_player()
				LOG(LOG_DEBUG, "[SetFieldHandler] Transferring player");
				Stage::get().transfer_player();
				LOG(LOG_DEBUG, "[SetFieldHandler] Player transferred");
				
				// Unlock graphics
				LOG(LOG_DEBUG, "[SetFieldHandler] Unlocking graphics");
				GraphicsGL::get().unlock();
				
				LOG(LOG_DEBUG, "[SetFieldHandler] Fadeout callback completed successfully");
				// Fadeout callback completed
			});
			
		LOG(LOG_DEBUG, "[SetFieldHandler] Fadeout scheduled");
		// Fadeout scheduled

		// No need to lock GL or clear stage here;
		// the fade callback does the cleanup.
	}

	void SetFieldHandler::handle(InPacket& recv) const
	{
		LOG(LOG_DEBUG, "[SetFieldHandler] Received SET_FIELD packet from server");
		// SetFieldHandler::handle() called
		Constants::Constants::get().set_viewwidth(Setting<Width>::get().load());
		Constants::Constants::get().set_viewheight(Setting<Height>::get().load());

		int32_t channel = recv.read_int();
		int8_t mode1 = recv.read_byte();
		int8_t mode2 = recv.read_byte();
		LOG(LOG_DEBUG, "[SetFieldHandler] Channel: " << channel << ", mode1: " << (int)mode1 << ", mode2: " << (int)mode2);
		// Channel, mode1, mode2 values read from packet

		if (mode1 == 0 && mode2 == 0) {
			LOG(LOG_DEBUG, "[SetFieldHandler] Calling change_map()");
			// Calling change_map()
			change_map(recv, channel);
		} else {
			LOG(LOG_DEBUG, "[SetFieldHandler] Calling set_field()");
			// Calling set_field()
			set_field(recv);
		}
		// SetFieldHandler::handle() completed
	}

	void SetFieldHandler::change_map(InPacket& recv, int32_t) const
	{
		recv.skip(3);

		int32_t mapid = recv.read_int();
		int8_t portalid = recv.read_byte();
		
		LOG(LOG_DEBUG, "[SetFieldHandler] change_map() - mapid: " << mapid << ", portalid: " << (int)portalid);

		transition(mapid, portalid);
	}

	void SetFieldHandler::set_field(InPacket& recv) const
	{
		LOG(LOG_DEBUG, "[SetFieldHandler] set_field() called");
		// SetFieldHandler::set_field() called
		recv.skip(23);

		int32_t cid = recv.read_int();
		LOG(LOG_DEBUG, "[SetFieldHandler] Character ID: " << cid);
		// Character ID read from packet
		
		auto charselect = UI::get().get_element<UICharSelect>();

		if (!charselect) {
			LOG(LOG_ERROR, "[SetFieldHandler] ERROR: UICharSelect not found!");
			return;
		}
		
		LOG(LOG_DEBUG, "[SetFieldHandler] Found UICharSelect");
		const CharEntry& playerentry = charselect->get_character(cid);
			
		if (playerentry.id != cid) {
			LOG(LOG_ERROR, "[SetFieldHandler] ERROR: Character ID mismatch - expected: " << cid << ", got: " << playerentry.id);
			return;
		}
		
		LOG(LOG_DEBUG, "[SetFieldHandler] Character ID match confirmed");
		
		// Loading player into stage
		LOG(LOG_DEBUG, "[SetFieldHandler] Loading player into stage");
		Stage::get().loadplayer(playerentry);
		LOG(LOG_DEBUG, "[SetFieldHandler] Player loaded into stage");

		// Remove all login-related UI before transitioning to game
		UI::get().remove(UIElement::Type::CHARSELECT);
		UI::get().remove(UIElement::Type::WORLDSELECT);
		UI::get().remove(UIElement::Type::LOGINNOTICE);
		UI::get().remove(UIElement::Type::RACESELECT);
		UI::get().remove(UIElement::Type::CLASSCREATION);

			// Starting to parse character stats
			StatsEntry newstats = LoginParser::parse_stats(recv);

			// Getting player reference
			Player& player = Stage::get().get_player();
			
			// Update player stats with the parsed values from server
			LOG(LOG_DEBUG, "[SetFieldHandler] Updating player stats from server");
			for (auto stat : newstats.stats) {
				if (stat.first == MapleStat::Id::HP || stat.first == MapleStat::Id::MAXHP) {
					LOG(LOG_DEBUG, "[SetFieldHandler] Setting stat " + std::to_string(static_cast<int>(stat.first)) + " to " + std::to_string(stat.second));
				}
				player.get_stats().set_stat(stat.first, stat.second);
			}
			player.get_stats().set_exp(newstats.exp);
			
			// Recalculate stats after updating base values
			player.recalc_stats(false);

			// Reading buddycap
			recv.read_byte(); // 'buddycap'

			// Checking linked name
			if (recv.read_bool())
				recv.read_string(); // 'linkedname'

			// Parsing inventory
			CharacterParser::parse_inventory(recv, player.get_inventory());
			
			// Parsing skillbook
			CharacterParser::parse_skillbook(recv, player.get_skills());
			
			// Parsing cooldowns
			CharacterParser::parse_cooldowns(recv, player);
			
			// Parsing questlog
			CharacterParser::parse_questlog(recv, player.get_quests());
			
			// Parsing minigame
			CharacterParser::parse_minigame(recv);
			
			// Parsing teleportrock
			CharacterParser::parse_teleportrock(recv, player.get_teleportrock());
			
			// Parsing monsterbook
			CharacterParser::parse_monsterbook(recv, player.get_monsterbook());
			
			// Parsing nyinfo
			LOG(LOG_DEBUG, "[SetFieldHandler] Parsing nyinfo");
			if (recv.length() > 2) {
				CharacterParser::parse_nyinfo(recv);
				LOG(LOG_DEBUG, "[SetFieldHandler] Nyinfo parsed");
			} else {
				LOG(LOG_DEBUG, "[SetFieldHandler] Skipping nyinfo - not enough data");
			}
			
			// Parsing areainfo
			LOG(LOG_DEBUG, "[SetFieldHandler] Parsing areainfo");
			if (recv.length() > 2) {
				CharacterParser::parse_areainfo(recv);
				LOG(LOG_DEBUG, "[SetFieldHandler] Areainfo parsed");
			} else {
				LOG(LOG_DEBUG, "[SetFieldHandler] Skipping areainfo - not enough data");
			}

			// Recalculating player stats
			LOG(LOG_DEBUG, "[SetFieldHandler] Recalculating player stats");
			player.recalc_stats(true);
			LOG(LOG_DEBUG, "[SetFieldHandler] Player stats recalculated");

			// Apply all equipped items to character's visual appearance
			LOG(LOG_DEBUG, "[SetFieldHandler] Applying equipped items to character appearance");
			for (int16_t slot = 1; slot <= EquipSlot::Id::LENGTH; slot++) {
				player.change_equip(slot);
			}
			LOG(LOG_DEBUG, "[SetFieldHandler] Equipment applied");

			// Dispatching PlayerUpdatePacket
			LOG(LOG_DEBUG, "[SetFieldHandler] Dispatching PlayerUpdatePacket");
			PlayerUpdatePacket().dispatch();
			LOG(LOG_DEBUG, "[SetFieldHandler] PlayerUpdatePacket dispatched");

			uint8_t portalid = player.get_stats().get_portal();
			int32_t mapid = player.get_stats().get_mapid();
			LOG(LOG_DEBUG, "[SetFieldHandler] Player stats - mapid: " << mapid << ", portalid: " << (int)portalid);

			// Validate map ID
			if (mapid == 0 || mapid > 999999999) {
				LOG(LOG_ERROR, "[SetFieldHandler] Invalid map ID: " << mapid << ", using default map");
				mapid = 100000000; // Henesys
				portalid = 0;
			}
			
			// Calling transition
			LOG(LOG_DEBUG, "[SetFieldHandler] Calling transition for mapid: " << mapid << ", portalid: " << (int)portalid);
			transition(mapid, portalid);

			// Playing game start sound
			Sound(Sound::Name::GAMESTART).play();

			// Changing UI state to GAME
			LOG(LOG_DEBUG, "[SetFieldHandler] Changing UI state to GAME");
			UI::get().change_state(UI::State::GAME);
			LOG(LOG_DEBUG, "[SetFieldHandler] set_field() completed");
	}
}