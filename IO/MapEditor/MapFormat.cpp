//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "MapFormat.h"

#include <algorithm>
#include <sstream>

namespace ms
{
	MapFormat::MapFormat() :
		width(20),
		height(15),
		return_map_id(100000000),  // Default to Henesys
		player_spawn_x(1),
		player_spawn_y(1)
	{
		ensure_grid_size();
	}
	
	MapFormat::~MapFormat()
	{
	}
	
	void MapFormat::set_size(int16_t w, int16_t h)
	{
		if (w > 0 && h > 0)
		{
			width = w;
			height = h;
			ensure_grid_size();
		}
	}
	
	void MapFormat::add_palette_entry(char symbol, const PaletteEntry& entry)
	{
		palette[symbol] = entry;
	}
	
	void MapFormat::remove_palette_entry(char symbol)
	{
		palette.erase(symbol);
		
		// Replace all instances in grid with empty space
		for (auto& row : grid)
		{
			std::replace(row.begin(), row.end(), symbol, ' ');
		}
	}
	
	PaletteEntry* MapFormat::get_palette_entry(char symbol)
	{
		auto it = palette.find(symbol);
		if (it != palette.end())
		{
			return &it->second;
		}
		return nullptr;
	}
	
	void MapFormat::set_tile(int16_t x, int16_t y, char symbol)
	{
		if (x >= 0 && x < width && y >= 0 && y < height)
		{
			grid[y][x] = symbol;
		}
	}
	
	char MapFormat::get_tile(int16_t x, int16_t y) const
	{
		if (x >= 0 && x < width && y >= 0 && y < height)
		{
			return grid[y][x];
		}
		return ' ';  // Empty space
	}
	
	void MapFormat::resize_grid(int16_t new_width, int16_t new_height)
	{
		if (new_width > 0 && new_height > 0)
		{
			// Create new grid with new dimensions
			std::vector<std::string> new_grid(new_height, std::string(new_width, ' '));
			
			// Copy existing data
			int16_t copy_width = std::min(width, new_width);
			int16_t copy_height = std::min(height, new_height);
			
			for (int16_t y = 0; y < copy_height; y++)
			{
				for (int16_t x = 0; x < copy_width; x++)
				{
					new_grid[y][x] = grid[y][x];
				}
			}
			
			// Update dimensions and grid
			width = new_width;
			height = new_height;
			grid = std::move(new_grid);
		}
	}
	
	void MapFormat::clear_tiles()
	{
		for (auto& row : grid)
		{
			std::fill(row.begin(), row.end(), ' ');
		}
	}
	
	void MapFormat::add_portal(const PortalData& portal)
	{
		portals.push_back(portal);
	}
	
	void MapFormat::add_npc(const NpcData& npc)
	{
		npcs.push_back(npc);
	}
	
	void MapFormat::add_mob_spawn(const MobSpawnData& spawn)
	{
		mob_spawns.push_back(spawn);
	}
	
	void MapFormat::set_player_spawn(int16_t x, int16_t y)
	{
		player_spawn_x = x;
		player_spawn_y = y;
	}
	
	bool MapFormat::is_valid() const
	{
		// Check basic constraints
		if (width <= 0 || height <= 0)
			return false;
			
		if (grid.size() != static_cast<size_t>(height))
			return false;
			
		for (const auto& row : grid)
		{
			if (row.length() != static_cast<size_t>(width))
				return false;
		}
		
		// Check that player spawn is within bounds
		if (player_spawn_x < 0 || player_spawn_x >= width ||
			player_spawn_y < 0 || player_spawn_y >= height)
			return false;
			
		// Check that all portals are within bounds
		for (const auto& portal : portals)
		{
			if (portal.x < 0 || portal.x >= width ||
				portal.y < 0 || portal.y >= height)
				return false;
		}
		
		return true;
	}
	
	std::string MapFormat::get_validation_errors() const
	{
		std::stringstream errors;
		
		if (width <= 0 || height <= 0)
			errors << "Invalid map dimensions\n";
			
		if (grid.size() != static_cast<size_t>(height))
			errors << "Grid height mismatch\n";
			
		for (size_t i = 0; i < grid.size(); i++)
		{
			if (grid[i].length() != static_cast<size_t>(width))
				errors << "Grid width mismatch at row " << i << "\n";
		}
		
		if (player_spawn_x < 0 || player_spawn_x >= width ||
			player_spawn_y < 0 || player_spawn_y >= height)
			errors << "Player spawn point out of bounds\n";
			
		for (size_t i = 0; i < portals.size(); i++)
		{
			if (portals[i].x < 0 || portals[i].x >= width ||
				portals[i].y < 0 || portals[i].y >= height)
				errors << "Portal " << i << " out of bounds\n";
		}
		
		return errors.str();
	}
	
	void MapFormat::clear()
	{
		map_name.clear();
		width = 20;
		height = 15;
		return_map_id = 100000000;
		palette.clear();
		grid.clear();
		portals.clear();
		npcs.clear();
		mob_spawns.clear();
		player_spawn_x = 1;
		player_spawn_y = 1;
		
		ensure_grid_size();
	}
	
	void MapFormat::ensure_grid_size()
	{
		grid.resize(height);
		for (auto& row : grid)
		{
			row.resize(width, ' ');
		}
	}
}