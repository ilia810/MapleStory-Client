//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "MapRenderer.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Graphics/Geometry.h"

namespace ms
{
	MapRenderer::MapRenderer() :
		tile_size(32),
		show_grid(true),
		camera_offset(0, 0)
	{
		// Initialize default tile colors (using uint8_t constructor)
		color_cache[' '] = Color(static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(255));  // Empty space - light gray
		color_cache['G'] = Color(static_cast<uint8_t>(34), static_cast<uint8_t>(139), static_cast<uint8_t>(34), static_cast<uint8_t>(255));    // Grass - forest green
		color_cache['T'] = Color(static_cast<uint8_t>(139), static_cast<uint8_t>(69), static_cast<uint8_t>(19), static_cast<uint8_t>(255));    // Tree - saddle brown
		color_cache['L'] = Color(static_cast<uint8_t>(160), static_cast<uint8_t>(82), static_cast<uint8_t>(45), static_cast<uint8_t>(255));    // Ladder - sienna
		color_cache['R'] = Color(static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(255));  // Rock - gray
		color_cache['W'] = Color(static_cast<uint8_t>(70), static_cast<uint8_t>(130), static_cast<uint8_t>(180), static_cast<uint8_t>(255));   // Water - steel blue
		color_cache['E'] = Color(static_cast<uint8_t>(178), static_cast<uint8_t>(34), static_cast<uint8_t>(34), static_cast<uint8_t>(255));    // Enemy/Mob - firebrick
		color_cache['N'] = Color(static_cast<uint8_t>(255), static_cast<uint8_t>(215), static_cast<uint8_t>(0), static_cast<uint8_t>(255));    // NPC - gold
		color_cache['P'] = Color(static_cast<uint8_t>(138), static_cast<uint8_t>(43), static_cast<uint8_t>(226), static_cast<uint8_t>(255));   // Portal - blue violet
		color_cache['S'] = Color(static_cast<uint8_t>(255), static_cast<uint8_t>(140), static_cast<uint8_t>(0), static_cast<uint8_t>(255));    // Player spawn - dark orange
	}
	
	MapRenderer::~MapRenderer()
	{
	}
	
	void MapRenderer::set_map(std::shared_ptr<MapFormat> map)
	{
		current_map = map;
		
		// Update color cache based on palette entries
		if (map)
		{
			const auto& palette = map->get_palette();
			for (const auto& entry : palette)
			{
				// Generate a color based on the palette entry name
				// In a real implementation, this would load actual tile graphics
				if (entry.second.name.find("Grass") != std::string::npos)
					color_cache[entry.first] = Color(static_cast<uint8_t>(34), static_cast<uint8_t>(139), static_cast<uint8_t>(34), static_cast<uint8_t>(255));
				else if (entry.second.name.find("Water") != std::string::npos)
					color_cache[entry.first] = Color(static_cast<uint8_t>(70), static_cast<uint8_t>(130), static_cast<uint8_t>(180), static_cast<uint8_t>(255));
				else if (entry.second.name.find("Tree") != std::string::npos)
					color_cache[entry.first] = Color(static_cast<uint8_t>(139), static_cast<uint8_t>(69), static_cast<uint8_t>(19), static_cast<uint8_t>(255));
				else if (entry.second.name.find("Rock") != std::string::npos)
					color_cache[entry.first] = Color(static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(128), static_cast<uint8_t>(255));
				else if (entry.second.name.find("Ladder") != std::string::npos)
					color_cache[entry.first] = Color(static_cast<uint8_t>(160), static_cast<uint8_t>(82), static_cast<uint8_t>(45), static_cast<uint8_t>(255));
			}
		}
	}
	
	void MapRenderer::render(Point<int16_t> position, float alpha) const
	{
		if (!current_map)
			return;
		
		// Apply camera offset
		Point<int16_t> render_pos = position - camera_offset;
		
		// Render tiles
		const auto& grid = current_map->get_grid();
		for (int16_t y = 0; y < current_map->get_height(); y++)
		{
			for (int16_t x = 0; x < current_map->get_width(); x++)
			{
				Point<int16_t> tile_pos(
					render_pos.x() + x * tile_size,
					render_pos.y() + y * tile_size
				);
				
				char symbol = current_map->get_tile(x, y);
				render_tile(tile_pos, symbol, alpha);
			}
		}
		
		// Render grid overlay
		if (show_grid)
			render_grid();
		
		// Render objects
		render_objects();
	}
	
	Point<int16_t> MapRenderer::get_dimensions() const
	{
		if (!current_map)
			return Point<int16_t>(0, 0);
		
		return Point<int16_t>(
			current_map->get_width() * tile_size,
			current_map->get_height() * tile_size
		);
	}
	
	Point<int16_t> MapRenderer::screen_to_tile(Point<int16_t> screen_pos) const
	{
		if (!current_map || tile_size == 0)
			return Point<int16_t>(-1, -1);
		
		Point<int16_t> adjusted_pos = screen_pos + camera_offset;
		
		int16_t tile_x = adjusted_pos.x() / tile_size;
		int16_t tile_y = adjusted_pos.y() / tile_size;
		
		// Check bounds
		if (tile_x < 0 || tile_x >= current_map->get_width() ||
			tile_y < 0 || tile_y >= current_map->get_height())
		{
			return Point<int16_t>(-1, -1);
		}
		
		return Point<int16_t>(tile_x, tile_y);
	}
	
	void MapRenderer::render_tile(Point<int16_t> tile_pos, char symbol, float alpha) const
	{
		Color tile_color = get_tile_color(symbol);
		float adjusted_alpha = tile_color.a() * alpha;
		
		// Draw filled rectangle for the tile
		GraphicsGL::get().drawrectangle(
			tile_pos.x(),
			tile_pos.y(),
			tile_size,
			tile_size,
			tile_color.r(),
			tile_color.g(),
			tile_color.b(),
			adjusted_alpha
		);
		
		// Draw tile symbol in center (for debugging)
		// In a real implementation, this would draw the actual tile sprite
		if (symbol != ' ')
		{
			// TODO: Draw text character in center of tile
		}
	}
	
	void MapRenderer::render_grid() const
	{
		if (!current_map)
			return;
		
		Point<int16_t> render_pos = Point<int16_t>(0, 0) - camera_offset;
		Color grid_color(static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(128));  // Semi-transparent gray
		
		// Draw vertical lines using thin rectangles
		for (int16_t x = 0; x <= current_map->get_width(); x++)
		{
			GraphicsGL::get().drawrectangle(
				render_pos.x() + x * tile_size,
				render_pos.y(),
				1,  // 1 pixel wide
				current_map->get_height() * tile_size,
				grid_color.r(),
				grid_color.g(),
				grid_color.b(),
				grid_color.a()
			);
		}
		
		// Draw horizontal lines using thin rectangles
		for (int16_t y = 0; y <= current_map->get_height(); y++)
		{
			GraphicsGL::get().drawrectangle(
				render_pos.x(),
				render_pos.y() + y * tile_size,
				current_map->get_width() * tile_size,
				1,  // 1 pixel tall
				grid_color.r(),
				grid_color.g(),
				grid_color.b(),
				grid_color.a()
			);
		}
	}
	
	void MapRenderer::render_objects() const
	{
		if (!current_map)
			return;
		
		Point<int16_t> render_pos = Point<int16_t>(0, 0) - camera_offset;
		
		// Render player spawn
		Point<int16_t> spawn_pos(
			render_pos.x() + current_map->get_player_spawn_x() * tile_size + tile_size / 2,
			render_pos.y() + current_map->get_player_spawn_y() * tile_size + tile_size / 2
		);
		
		// Draw spawn marker (orange square)
		Color spawn_color(static_cast<uint8_t>(255), static_cast<uint8_t>(140), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
		GraphicsGL::get().drawrectangle(
			spawn_pos.x() - 8,
			spawn_pos.y() - 8,
			16,
			16,
			spawn_color.r(),
			spawn_color.g(),
			spawn_color.b(),
			spawn_color.a()
		);
		
		// Render portals
		const auto& portals = current_map->get_portals();
		for (const auto& portal : portals)
		{
			Point<int16_t> portal_pos(
				render_pos.x() + portal.x * tile_size + tile_size / 2,
				render_pos.y() + portal.y * tile_size + tile_size / 2
			);
			
			// Draw portal marker (blue violet square)
			uint8_t alpha = (portal.type == "hidden") ? 128 : 255;
			Color portal_color(static_cast<uint8_t>(138), static_cast<uint8_t>(43), static_cast<uint8_t>(226), alpha);
			
			// Draw a rotated square to make diamond shape (simplified to square for now)
			GraphicsGL::get().drawrectangle(
				portal_pos.x() - 10,
				portal_pos.y() - 10,
				20,
				20,
				portal_color.r(),
				portal_color.g(),
				portal_color.b(),
				portal_color.a()
			);
		}
		
		// Render NPCs
		const auto& npcs = current_map->get_npcs();
		for (const auto& npc : npcs)
		{
			Point<int16_t> npc_pos(
				render_pos.x() + npc.x * tile_size + tile_size / 2,
				render_pos.y() + npc.y * tile_size + tile_size / 2
			);
			
			// Draw NPC marker (gold square)
			Color npc_color(static_cast<uint8_t>(255), static_cast<uint8_t>(215), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
			GraphicsGL::get().drawrectangle(
				npc_pos.x() - 6,
				npc_pos.y() - 6,
				12,
				12,
				npc_color.r(),
				npc_color.g(),
				npc_color.b(),
				npc_color.a()
			);
		}
		
		// Render mob spawns
		const auto& mobs = current_map->get_mob_spawns();
		for (const auto& mob : mobs)
		{
			Point<int16_t> mob_pos(
				render_pos.x() + mob.x * tile_size + tile_size / 2,
				render_pos.y() + mob.y * tile_size + tile_size / 2
			);
			
			// Draw mob marker (red triangle - simplified to square)
			Color mob_color(static_cast<uint8_t>(178), static_cast<uint8_t>(34), static_cast<uint8_t>(34), static_cast<uint8_t>(255));
			// Draw as a square for now since we don't have triangle drawing
			GraphicsGL::get().drawrectangle(
				mob_pos.x() - 7,
				mob_pos.y() - 7,
				14,
				14,
				mob_color.r(),
				mob_color.g(),
				mob_color.b(),
				mob_color.a()
			);
		}
	}
	
	Color MapRenderer::get_tile_color(char symbol) const
	{
		auto it = color_cache.find(symbol);
		if (it != color_cache.end())
			return it->second;
		
		// Default color for unknown tiles
		return Color(static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(255));
	}
}