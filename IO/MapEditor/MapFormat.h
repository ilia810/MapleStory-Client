//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace ms
{
	// Represents a single palette entry (e.g., G = Grass)
	struct PaletteEntry
	{
		int32_t id;			// MapleStory object ID
		std::string name;	// Human-readable name
		
		PaletteEntry() : id(0), name("") {}
		PaletteEntry(int32_t id, const std::string& name) : id(id), name(name) {}
	};
	
	// Represents a portal in the map
	struct PortalData
	{
		int16_t x;
		int16_t y;
		std::string destination_map;
		int32_t destination_id;
		std::string type;  // "regular", "hidden", "script"
		
		PortalData() : x(0), y(0), destination_id(0), type("regular") {}
	};
	
	// Represents an NPC placement
	struct NpcData
	{
		int32_t id;
		int16_t x;
		int16_t y;
		bool flip;  // facing direction
		
		NpcData() : id(0), x(0), y(0), flip(false) {}
	};
	
	// Represents a mob spawn point
	struct MobSpawnData
	{
		int32_t id;
		int16_t x;
		int16_t y;
		int32_t respawn_time;  // in seconds
		
		MobSpawnData() : id(0), x(0), y(0), respawn_time(30) {}
	};
	
	// Main map format structure
	class MapFormat
	{
	public:
		MapFormat();
		~MapFormat();
		
		// Map metadata
		void set_name(const std::string& name) { map_name = name; }
		void set_size(int16_t width, int16_t height);
		void set_return_map(int32_t map_id) { return_map_id = map_id; }
		
		const std::string& get_name() const { return map_name; }
		int16_t get_width() const { return width; }
		int16_t get_height() const { return height; }
		int32_t get_return_map() const { return return_map_id; }
		
		// Palette management
		void add_palette_entry(char symbol, const PaletteEntry& entry);
		void remove_palette_entry(char symbol);
		PaletteEntry* get_palette_entry(char symbol);
		const std::map<char, PaletteEntry>& get_palette() const { return palette; }
		
		// Grid management
		void set_tile(int16_t x, int16_t y, char symbol);
		char get_tile(int16_t x, int16_t y) const;
		void resize_grid(int16_t new_width, int16_t new_height);
		void clear_tiles();
		const std::vector<std::string>& get_grid() const { return grid; }
		
		// Object management
		void add_portal(const PortalData& portal);
		void add_npc(const NpcData& npc);
		void add_mob_spawn(const MobSpawnData& spawn);
		void set_player_spawn(int16_t x, int16_t y);
		
		const std::vector<PortalData>& get_portals() const { return portals; }
		const std::vector<NpcData>& get_npcs() const { return npcs; }
		const std::vector<MobSpawnData>& get_mob_spawns() const { return mob_spawns; }
		int16_t get_player_spawn_x() const { return player_spawn_x; }
		int16_t get_player_spawn_y() const { return player_spawn_y; }
		int16_t& get_mutable_player_spawn_x() { return player_spawn_x; }
		int16_t& get_mutable_player_spawn_y() { return player_spawn_y; }
		
		// Validation
		bool is_valid() const;
		std::string get_validation_errors() const;
		
		// Clear all data
		void clear();
		
	private:
		// Map metadata
		std::string map_name;
		int16_t width;
		int16_t height;
		int32_t return_map_id;
		
		// Palette: symbol -> tile/object data
		std::map<char, PaletteEntry> palette;
		
		// 2D grid of symbols
		std::vector<std::string> grid;
		
		// Objects
		std::vector<PortalData> portals;
		std::vector<NpcData> npcs;
		std::vector<MobSpawnData> mob_spawns;
		int16_t player_spawn_x;
		int16_t player_spawn_y;
		
		// Helper to ensure grid has correct dimensions
		void ensure_grid_size();
	};
}