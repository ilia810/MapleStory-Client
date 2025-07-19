//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "MapFormat.h"
#include "../../Graphics/Texture.h"
#include "../../Template/Point.h"
#include <memory>
#include <map>

namespace ms
{
	class MapRenderer
	{
	public:
		MapRenderer();
		~MapRenderer();
		
		// Set the map to render
		void set_map(std::shared_ptr<MapFormat> map);
		
		// Render the map at the given position
		void render(Point<int16_t> position, float alpha = 1.0f) const;
		
		// Set the tile size for rendering
		void set_tile_size(int16_t size) { tile_size = size; }
		
		// Get the rendered map dimensions in pixels
		Point<int16_t> get_dimensions() const;
		
		// Set grid visibility
		void set_grid_visible(bool visible) { show_grid = visible; }
		
		// Set camera offset for panning
		void set_camera_offset(Point<int16_t> offset) { camera_offset = offset; }
		
		// Get tile at screen position
		Point<int16_t> screen_to_tile(Point<int16_t> screen_pos) const;
		
		// Get color for a palette symbol
		Color get_tile_color(char symbol) const;
		
	private:
		// Render a single tile
		void render_tile(Point<int16_t> tile_pos, char symbol, float alpha) const;
		
		// Render the grid overlay
		void render_grid() const;
		
		// Render objects (portals, NPCs, mobs)
		void render_objects() const;
		
		std::shared_ptr<MapFormat> current_map;
		int16_t tile_size;
		bool show_grid;
		Point<int16_t> camera_offset;
		
		// Cache for tile textures (in a real implementation)
		mutable std::map<char, Color> color_cache;
	};
}