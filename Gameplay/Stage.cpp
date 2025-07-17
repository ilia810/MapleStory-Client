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
#include "Stage.h"

#include "../Configuration.h"

#include "../IO/UI.h"

#include <iostream>

#include "../IO/UITypes/UIStatusBar.h"
#include "../Net/Packets/AttackAndSkillPackets.h"
#include "../Net/Packets/GameplayPackets.h"
#include "../Util/Misc.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Stage::Stage() : combat(player, chars, mobs, reactors)
	{
		state = State::INACTIVE;
	}

	void Stage::init()
	{
		drops.init();
	}

	void Stage::load(int32_t mapid, int8_t portalid)
	{
		LOG(LOG_DEBUG, "[Stage] load() called - mapid: " << mapid << ", portalid: " << (int)portalid << ", current state: " << (int)state);
		
		// For map transitions, we need to load the new map even if active
		bool is_new_map = (mapid != Stage::mapid);
		
		if (is_new_map) {
			// Force state to allow loading new map
			state = State::TRANSITION;
		}
		
		switch (state)
		{
			case State::INACTIVE:
					load_map(mapid);
				respawn(portalid);
				break;
			case State::TRANSITION:
					if (is_new_map) {
						clear();  // Clear old map data
						load_map(mapid);
					}
					respawn(portalid);
				break;
			case State::ACTIVE:
				// Already active, skip loading
				LOG(LOG_DEBUG, "[Stage] Already active, skipping load");
				return;
		}

		state = State::ACTIVE;
		LOG(LOG_DEBUG, "[Stage] Map loaded successfully, state set to ACTIVE");
		
		// Process any pending NPC spawns that might have been received during transition
		npcs.update(physics);
		
		// Give the NPCs another update cycle in case some were queued
		npcs.update(physics);
	}

	void Stage::loadplayer(const CharEntry& entry)
	{
		player = entry;
		playable = player;

		start = ContinuousTimer::get().start();

		CharStats stats = player.get_stats();
		levelBefore = stats.get_stat(MapleStat::Id::LEVEL);
		expBefore = stats.get_exp();
	}

	void Stage::clear()
	{
		state = State::INACTIVE;

		chars.clear();
		npcs.clear();
		mobs.clear();
		drops.clear();
		reactors.clear();
	}

	void Stage::load_map(int32_t mapid)
	{
		Stage::mapid = mapid;
		LOG(LOG_DEBUG, "[Stage] Loading map: " << mapid);

		std::string strid = string_format::extend_id(mapid, 9);
		std::string prefix = std::to_string(mapid / 100000000);
		LOG(LOG_DEBUG, "[Stage] Map file path: Map" << prefix << "/" << strid << ".img");

		// Try Map002 fallback, then direct Map (v83)
		nl::node src;
		if (mapid == -1) {
			src = nl::nx::UI["CashShopPreview.img"];
		} else {
			src = nl::nx::Map002["Map"]["Map" + prefix][strid + ".img"];
			if (src.name().empty()) {
				src = nl::nx::Map["Map"]["Map" + prefix][strid + ".img"];
			}
			
			// Fix: Fallback again if critical nodes are missing (v87 compatibility)
			if (src["portal"].size() == 0 || src["life"].size() == 0) {
				nl::node alt = nl::nx::Map["Map"]["Map" + prefix][strid + ".img"];
				if (!alt.name().empty()) {
					src = alt;  // use classic map that contains full data
				}
			}
		}

		tilesobjs = MapTilesObjs(src);
		backgrounds = MapBackgrounds(src["back"]);
		
		// Debug: Check if foothold data exists
		nl::node foothold_node = src["foothold"];
		if (foothold_node.name().empty()) {
			LOG(LOG_ERROR, "[Stage] No foothold data found for map " << mapid << " - players will fall through!");
		} else {
			int foothold_count = 0;
			for (auto group : foothold_node) {
				for (auto cat : group) {
					for (auto fh : cat) {
						foothold_count++;
					}
				}
			}
			LOG(LOG_DEBUG, "[Stage] Found " << foothold_count << " footholds for map " << mapid);
		}
		
		physics = Physics(src["foothold"]);
		mapinfo = MapInfo(src, physics.get_fht().get_walls(), physics.get_fht().get_borders());
		
		// Debug: Check portal data
		nl::node portal_node = src["portal"];
		if (portal_node.name().empty()) {
			LOG(LOG_ERROR, "[Stage] No portal data found for map " << mapid << " - spawn will fail!");
		}
		
		portals = MapPortals(src["portal"], mapid);
		LOG(LOG_DEBUG, "[Stage] Loaded " << portals.get_portal_count() << " portals for map " << mapid);
		
		// Check if map has NPC life data that should be loaded
		nl::node life_node = src["life"];
		if (!life_node.name().empty()) {
			int npc_count = 0;
			for (auto life_entry : life_node) {
				std::string type = life_entry["type"];
				if (type == "n") { // NPC
					npc_count++;
					LOG(LOG_DEBUG, "[Stage] Found NPC in map data: ID=" << (int)life_entry["id"] 
						<< ", pos=(" << (int)life_entry["x"] << "," << (int)life_entry["y"] << ")");
				}
			}
			LOG(LOG_DEBUG, "[Stage] Map " << mapid << " has " << npc_count << " NPCs in life data (waiting for server spawn packets)");
		} else {
			LOG(LOG_DEBUG, "[Stage] Map " << mapid << " has no life data node");
		}
		
		// NPCs are loaded through network packets from the server, not from map data
	}

	void Stage::respawn(int8_t portalid)
	{
		Music(mapinfo.get_bgm()).play();

		LOG(LOG_DEBUG, "[Stage] Respawning player at portal ID: " << (int)portalid << " on map " << mapid);
		
		// Debug: Check portal count and validity
		int portal_count = portals.get_portal_count();
		LOG(LOG_DEBUG, "[Stage] Map has " << portal_count << " portals available");
		
		Point<int16_t> spawnpoint = portals.get_portal_by_id(portalid);
		LOG(LOG_DEBUG, "[Stage] Portal spawnpoint: (" << spawnpoint.x() << "," << spawnpoint.y() << ")");
		
		// Debug: Check if portal returned (0,0) - indicates missing portal
		if (spawnpoint.x() == 0 && spawnpoint.y() == 0) {
			LOG(LOG_ERROR, "[Stage] Portal " << (int)portalid << " returned (0,0) - portal missing or invalid!");
			LOG(LOG_ERROR, "[Stage] Attempting to use portal 0 as fallback");
			spawnpoint = portals.get_portal_by_id(0);
			LOG(LOG_DEBUG, "[Stage] Fallback portal 0 position: (" << spawnpoint.x() << "," << spawnpoint.y() << ")");
		}
		
		Point<int16_t> startpos = physics.get_y_below(spawnpoint);
		LOG(LOG_DEBUG, "[Stage] Physics calculated startpos: (" << startpos.x() << "," << startpos.y() << ")");
		
		// Debug: Check if physics returned invalid position
		if (startpos.y() == spawnpoint.y()) {
			LOG(LOG_ERROR, "[Stage] Physics returned same Y position - no foothold found below spawn point!");
			LOG(LOG_ERROR, "[Stage] This indicates missing or corrupted foothold data");
		}
		
		// Fix invalid spawn position
		if (startpos.y() < -1000 || startpos.y() > 2000) {
			LOG(LOG_ERROR, "[Stage] Invalid spawn position detected: (" << startpos.x() << "," << startpos.y() << ")");
			startpos = Point<int16_t>(startpos.x(), 300);
			LOG(LOG_DEBUG, "[Stage] Fixed invalid spawn position to: (" << startpos.x() << "," << startpos.y() << ")");
		}
		
		player.respawn(startpos, mapinfo.is_underwater());
		LOG(LOG_DEBUG, "[Stage] Player respawned at final position: (" << startpos.x() << "," << startpos.y() << ")");
		// Center camera on player position
		camera.set_position(startpos);
		Range<int16_t> walls = mapinfo.get_walls();
		Range<int16_t> borders = mapinfo.get_borders();
		
		LOG(LOG_DEBUG, "[Stage] MAP ID: " << mapid << " - Camera bounds - walls: " << walls.smaller() << " to " << walls.greater() 
			<< ", borders: " << borders.smaller() << " to " << borders.greater());
		
		// Use the VR bounds calculated in MapInfo which handles missing VR nodes
		camera.set_view(walls, borders);
	}

	void Stage::draw(float alpha) const
	{
		if (state != State::ACTIVE) {
			// Stage is not active - don't draw anything during login screens
			return;
		}

		Point<int16_t> viewpos = camera.position(alpha);
		Point<double> viewrpos = camera.realposition(alpha);
		// Use actual camera position instead of hardcoded values
		double viewx = viewrpos.x();
		double viewy = viewrpos.y();

		backgrounds.drawbackgrounds(viewx, viewy, alpha);

		static int draw_frame = 0;
		bool debug_this_frame = (draw_frame++ % 600 == 0); // Log every 10 seconds
		
		for (auto id : Layer::IDs)
		{
			tilesobjs.draw(id, viewpos, alpha);
			reactors.draw(id, viewx, viewy, alpha);
			npcs.draw(id, viewx, viewy, alpha);
			mobs.draw(id, viewx, viewy, alpha);
			chars.draw(id, viewx, viewy, alpha);
			player.draw(id, viewx, viewy, alpha);
			drops.draw(id, viewx, viewy, alpha);
		}

		combat.draw(viewx, viewy, alpha);
		portals.draw(viewpos, alpha);
		backgrounds.drawforegrounds(viewx, viewy, alpha);
		effect.draw();
	}

	void Stage::update()
	{
		static int stage_update_count = 0;
		// if (stage_update_count++ % 60 == 0) {
		//	printf("[Stage::update] Called #%d, state=%d (ACTIVE=%d)\n", stage_update_count, (int)state, (int)State::ACTIVE);
		// }
		
		if (state != State::ACTIVE) {
			// LOG(LOG_DEBUG, "[Stage] update() skipped - state is not ACTIVE (state=" << (int)state << ")");
			return;
		}

		combat.update();
		backgrounds.update();
		effect.update();
		tilesobjs.update();

		reactors.update(physics);
		npcs.update(physics);
		mobs.update(physics);
		chars.update(physics);
		drops.update(physics);
		player.update(physics);

		portals.update(player.get_position());
		Point<int16_t> player_pos = player.get_position();
		camera.update(player_pos);

		if (!player.is_climbing() && !player.is_sitting() && !player.is_attacking())
		{
			if (player.is_key_down(KeyAction::Id::UP) && !player.is_key_down(KeyAction::Id::DOWN))
				check_ladders(true);

			if (player.is_key_down(KeyAction::Id::UP))
				check_portals();

			if (player.is_key_down(KeyAction::Id::DOWN))
				check_ladders(false);

			if (player.is_key_down(KeyAction::Id::SIT))
				check_seats();

			if (player.is_key_down(KeyAction::Id::ATTACK))
				combat.use_move(0);

			if (player.is_key_down(KeyAction::Id::PICKUP))
				check_drops();
		}

		if (player.is_invincible())
			return;

		if (int32_t oid_id = mobs.find_colliding(player.get_phobj()))
		{
			if (MobAttack attack = mobs.create_attack(oid_id))
			{
				MobAttackResult result = player.damage(attack);
				TakeDamagePacket(result, TakeDamagePacket::From::TOUCH).dispatch();
			}
		}
	}

	void Stage::show_character_effect(int32_t cid, CharEffect::Id effect)
	{
		if (auto character = get_character(cid))
			character->show_effect_id(effect);
	}

	void Stage::check_portals()
	{
		if (player.is_attacking())
			return;

		Point<int16_t> playerpos = player.get_position();
		// LOG(LOG_DEBUG, "[Stage] check_portals() - Player position: " << playerpos.x() << "," << playerpos.y() << " on map " << mapid);
		
		Portal::WarpInfo warpinfo = portals.find_warp_at(playerpos);

		if (warpinfo.intramap)
		{
			LOG(LOG_DEBUG, "[Stage] Intramap portal found - to: " << warpinfo.toname);
			Point<int16_t> spawnpoint = portals.get_portal_by_name(warpinfo.toname);
			
			if (spawnpoint.x() == 0 && spawnpoint.y() == 0) {
				LOG(LOG_DEBUG, "[Stage] WARNING: Target portal '" << warpinfo.toname << "' not found for intramap teleport!");
				return;
			}
			
			Point<int16_t> startpos = physics.get_y_below(spawnpoint);
			LOG(LOG_DEBUG, "[Stage] Intramap teleport: spawnpoint=(" << spawnpoint.x() << "," << spawnpoint.y() 
				<< "), startpos=(" << startpos.x() << "," << startpos.y() << ")");

			player.respawn(startpos, mapinfo.is_underwater());
		}
		else if (warpinfo.valid)
		{
			LOG(LOG_DEBUG, "[Stage] Portal found - name: " << warpinfo.name << ", to map: " << warpinfo.mapid);
			ChangeMapPacket(false, warpinfo.mapid, warpinfo.name, false).dispatch();

			CharStats& stats = Stage::get().get_player().get_stats();

			stats.set_mapid(warpinfo.mapid);

			Sound(Sound::Name::PORTAL).play();
			LOG(LOG_DEBUG, "[Stage] Sent ChangeMapPacket to server");
		}
		else
		{
			// LOG(LOG_DEBUG, "[Stage] No portal found at player position");
		}
	}

	void Stage::check_seats()
	{
		if (player.is_sitting() || player.is_attacking())
			return;

		Optional<const Seat> seat = mapinfo.findseat(player.get_position());
		player.set_seat(seat);
	}

	void Stage::check_ladders(bool up)
	{
		if (!player.can_climb() || player.is_climbing() || player.is_attacking())
			return;

		Optional<const Ladder> ladder = mapinfo.findladder(player.get_position(), up);
		player.set_ladder(ladder);
	}

	void Stage::check_drops()
	{
		Point<int16_t> playerpos = player.get_position();
		MapDrops::Loot loot = drops.find_loot_at(playerpos);

		if (loot.first)
			PickupItemPacket(loot.first, loot.second).dispatch();
	}

	void Stage::send_key(KeyType::Id type, int32_t action, bool down)
	{
		if (state != State::ACTIVE || !playable)
			return;

		switch (type)
		{
			case KeyType::Id::ACTION:
				playable->send_action(KeyAction::actionbyid(action), down);
				break;
			case KeyType::Id::SKILL:
				combat.use_move(action);
				break;
			case KeyType::Id::ITEM:
				player.use_item(action);
				break;
			case KeyType::Id::FACE:
				player.set_expression(action);
				break;
		}
	}

	Cursor::State Stage::send_cursor(bool clicked, Point<int16_t> cursor_position)
	{
		auto statusbar = UI::get().get_element<UIStatusBar>();

		if (statusbar && statusbar->is_menu_active())
		{
			if (clicked)
				statusbar->remove_menus();

			if (statusbar->is_in_range(cursor_position))
				return statusbar->send_cursor(clicked, cursor_position);
		}

		return npcs.send_cursor(clicked, cursor_position, camera.position());
	}

	bool Stage::is_player(int32_t cid) const
	{
		return cid == player.get_oid();
	}

	MapNpcs& Stage::get_npcs()
	{
		return npcs;
	}

	MapChars& Stage::get_chars()
	{
		return chars;
	}

	MapMobs& Stage::get_mobs()
	{
		return mobs;
	}

	MapReactors& Stage::get_reactors()
	{
		return reactors;
	}

	MapDrops& Stage::get_drops()
	{
		return drops;
	}

	Player& Stage::get_player()
	{
		return player;
	}

	Combat& Stage::get_combat()
	{
		return combat;
	}

	Optional<Char> Stage::get_character(int32_t cid)
	{
		if (is_player(cid))
			return player;
		else
			return chars.get_char(cid);
	}

	int Stage::get_mapid()
	{
		return mapid;
	}

	void Stage::add_effect(std::string path)
	{
		effect = MapEffect(path);
	}

	int64_t Stage::get_uptime()
	{
		return ContinuousTimer::get().stop(start);
	}

	uint16_t Stage::get_uplevel()
	{
		return levelBefore;
	}

	int64_t Stage::get_upexp()
	{
		return expBefore;
	}
	
	bool Stage::is_transitioning() const
	{
		return state != State::ACTIVE;
	}
	
	int32_t Stage::get_current_mapid() const
	{
		return mapid;
	}

	void Stage::transfer_player()
	{
		PlayerMapTransferPacket().dispatch();

		if (Configuration::get().get_admin())
			AdminEnterMapPacket(AdminEnterMapPacket::Operation::ALERT_ADMINS).dispatch();
	}
}