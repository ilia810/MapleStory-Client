//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "UITilePalette.h"
#include "MapRenderer.h"
#include "../../Graphics/GraphicsGL.h"
#include "../Components/Button.h"

namespace ms
{
	UITilePalette::UITilePalette() :
		UIDragElement<PosTILEPALETTE>(Point<int16_t>(PALETTE_WIDTH, HEADER_HEIGHT)),
		current_palette(nullptr),
		selected_tile(' '),
		scroll_offset(0),
		title(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK),
		selected_info(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::BLACK)
	{
		// Set dimension - position is handled by UIDragElement
		dimension = Point<int16_t>(PALETTE_WIDTH, PALETTE_HEIGHT);
		active = true;
		
		title.change_text("Tile Palette");
		selected_info.change_text("Selected: None");
		
		// For now, skip button creation since MapleButton requires NX data
		// TODO: Create custom simple button class for map editor
	}
	
	void UITilePalette::draw(float inter) const
	{
		// Draw background
		Color bg_color(static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(230));
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			dimension.y(),
			bg_color.r(),
			bg_color.g(),
			bg_color.b(),
			bg_color.a()
		);
		
		// Draw border
		Color border_color(static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(100), static_cast<uint8_t>(255));
		// Top border
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			2,
			border_color.r(),
			border_color.g(),
			border_color.b(),
			border_color.a()
		);
		// Bottom border
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y() + dimension.y() - 2,
			dimension.x(),
			2,
			border_color.r(),
			border_color.g(),
			border_color.b(),
			border_color.a()
		);
		// Left border
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			2,
			dimension.y(),
			border_color.r(),
			border_color.g(),
			border_color.b(),
			border_color.a()
		);
		// Right border
		GraphicsGL::get().drawrectangle(
			position.x() + dimension.x() - 2,
			position.y(),
			2,
			dimension.y(),
			border_color.r(),
			border_color.g(),
			border_color.b(),
			border_color.a()
		);
		
		// Draw header
		Color header_color(static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(200), static_cast<uint8_t>(255));
		GraphicsGL::get().drawrectangle(
			position.x(),
			position.y(),
			dimension.x(),
			HEADER_HEIGHT,
			header_color.r(),
			header_color.g(),
			header_color.b(),
			header_color.a()
		);
		
		// Draw title
		title.draw(DrawArgument(position + Point<int16_t>(dimension.x() / 2, 15)));
		
		// Draw palette tiles
		if (current_palette)
		{
			size_t index = 0;
			for (const auto& entry : *current_palette)
			{
				Point<int16_t> tile_pos = position + get_tile_position(index) + Point<int16_t>(0, HEADER_HEIGHT - scroll_offset);
				
				// Only draw if visible
				if (tile_pos.y() >= position.y() + HEADER_HEIGHT && 
					tile_pos.y() + TILE_SIZE <= position.y() + dimension.y())
				{
					// Get color for this tile
					MapRenderer renderer;
					Color tile_color = renderer.get_tile_color(entry.first);
					
					// Draw tile background
					GraphicsGL::get().drawrectangle(
						tile_pos.x(),
						tile_pos.y(),
						TILE_SIZE,
						TILE_SIZE,
						tile_color.r(),
						tile_color.g(),
						tile_color.b(),
						tile_color.a()
					);
					
					// Draw selection highlight
					if (entry.first == selected_tile)
					{
						Color highlight_color(static_cast<uint8_t>(255), static_cast<uint8_t>(255), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
						// Draw selection border (4 rectangles)
						GraphicsGL::get().drawrectangle(tile_pos.x() - 2, tile_pos.y() - 2, TILE_SIZE + 4, 2, highlight_color.r(), highlight_color.g(), highlight_color.b(), highlight_color.a());
						GraphicsGL::get().drawrectangle(tile_pos.x() - 2, tile_pos.y() + TILE_SIZE, TILE_SIZE + 4, 2, highlight_color.r(), highlight_color.g(), highlight_color.b(), highlight_color.a());
						GraphicsGL::get().drawrectangle(tile_pos.x() - 2, tile_pos.y() - 2, 2, TILE_SIZE + 4, highlight_color.r(), highlight_color.g(), highlight_color.b(), highlight_color.a());
						GraphicsGL::get().drawrectangle(tile_pos.x() + TILE_SIZE, tile_pos.y() - 2, 2, TILE_SIZE + 4, highlight_color.r(), highlight_color.g(), highlight_color.b(), highlight_color.a());
					}
					
					// Draw tile symbol (would be actual sprite in full implementation)
					// For now, just draw a darker rectangle with the symbol
					if (entry.first != ' ')
					{
						Color symbol_color(static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(100));
						GraphicsGL::get().drawrectangle(
							tile_pos.x() + TILE_SIZE/3,
							tile_pos.y() + TILE_SIZE/3,
							TILE_SIZE/3,
							TILE_SIZE/3,
							symbol_color.r(),
							symbol_color.g(),
							symbol_color.b(),
							symbol_color.a()
						);
					}
				}
				
				index++;
			}
		}
		
		// Draw selected info at bottom
		selected_info.draw(DrawArgument(position + Point<int16_t>(5, dimension.y() - 20)));
		
		// Draw buttons
		// TODO: Implement when button class is ready
		/*for (const auto& button : buttons)
		{
			if (button.second)
				button.second->draw(position);
		}*/
	}
	
	void UITilePalette::update()
	{
		// TODO: Update buttons when implemented
		/*for (auto& button : buttons)
		{
			if (button.second)
				button.second->update();
		}*/
	}
	
	void UITilePalette::send_scroll(double yoffset)
	{
		scroll_offset += static_cast<int16_t>(yoffset * 20);
		
		// Clamp scroll offset
		if (scroll_offset < 0)
			scroll_offset = 0;
		
		if (current_palette)
		{
			int16_t max_scroll = static_cast<int16_t>(
				((current_palette->size() + TILES_PER_ROW - 1) / TILES_PER_ROW) * (TILE_SIZE + TILE_SPACING) 
				- (dimension.y() - HEADER_HEIGHT - 30)
			);
			if (max_scroll < 0)
				max_scroll = 0;
			if (scroll_offset > max_scroll)
				scroll_offset = max_scroll;
		}
	}
	
	Cursor::State UITilePalette::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// First, handle dragging by calling parent implementation
		Cursor::State drag_state = UIDragElement<PosTILEPALETTE>::send_cursor(clicked, cursorpos);
		
		// If we're currently dragging, don't handle tile selection
		if (dragged)
		{
			return drag_state;
		}
		
		// Check for tile selection only if not dragging and cursor is in tile area
		if (clicked && current_palette)
		{
			Point<int16_t> relative_pos = cursorpos - position;
			if (relative_pos.y() > HEADER_HEIGHT && relative_pos.y() < dimension.y() - 30)
			{
				char tile = get_tile_at_position(cursorpos);
				if (tile != '\0')
				{
					selected_tile = tile;
					
					// Update selected info text
					auto it = current_palette->find(selected_tile);
					if (it != current_palette->end())
					{
						selected_info.change_text("Selected: " + std::string(1, selected_tile) + " - " + it->second.name);
					}
					else
					{
						selected_info.change_text("Selected: " + std::string(1, selected_tile));
					}
					
					return Cursor::State::CLICKING;
				}
			}
		}
		
		return drag_state;
	}
	
	void UITilePalette::set_palette(const std::map<char, PaletteEntry>* palette)
	{
		current_palette = palette;
		scroll_offset = 0;
		
		if (palette && !palette->empty())
		{
			// Select first tile by default
			selected_tile = palette->begin()->first;
			selected_info.change_text("Selected: " + std::string(1, selected_tile) + " - " + palette->begin()->second.name);
		}
	}
	
	void UITilePalette::add_custom_tile(char symbol, int32_t id, const std::string& name)
	{
		// This would add a custom tile to the palette
		// Implementation would modify the map format's palette
	}
	
	Button::State UITilePalette::button_pressed(uint16_t buttonid)
	{
		switch (buttonid)
		{
		case BT_CLOSE:
			active = false;
			return Button::State::NORMAL;
		case BT_ADD_TILE:
			// TODO: Show dialog to add custom tile
			return Button::State::NORMAL;
		case BT_REMOVE_TILE:
			// TODO: Remove selected tile from palette
			return Button::State::NORMAL;
		default:
			return Button::State::NORMAL;
		}
	}
	
	void UITilePalette::update_layout()
	{
		// Recalculate layout if needed
	}
	
	Point<int16_t> UITilePalette::get_tile_position(size_t index) const
	{
		int16_t row = static_cast<int16_t>(index / TILES_PER_ROW);
		int16_t col = static_cast<int16_t>(index % TILES_PER_ROW);
		
		return Point<int16_t>(
			TILE_SPACING + col * (TILE_SIZE + TILE_SPACING),
			TILE_SPACING + row * (TILE_SIZE + TILE_SPACING)
		);
	}
	
	char UITilePalette::get_tile_at_position(Point<int16_t> pos) const
	{
		if (!current_palette)
			return '\0';
		
		Point<int16_t> relative_pos = pos - position - Point<int16_t>(0, HEADER_HEIGHT - scroll_offset);
		
		// Check each tile
		size_t index = 0;
		for (const auto& entry : *current_palette)
		{
			Point<int16_t> tile_pos = get_tile_position(index);
			Rectangle<int16_t> tile_bounds(
				tile_pos.x(),
				tile_pos.x() + TILE_SIZE,
				tile_pos.y(),
				tile_pos.y() + TILE_SIZE
			);
			
			if (tile_bounds.contains(relative_pos))
			{
				return entry.first;
			}
			
			index++;
		}
		
		return '\0';
	}
}