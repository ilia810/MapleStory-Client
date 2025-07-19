//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIDragElement.h"
#include "../Components/Button.h"
#include "MapFormat.h"
#include "../../Graphics/Text.h"
#include "../../Configuration.h"

namespace ms
{
	class UITilePalette : public UIDragElement<PosTILEPALETTE>
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAP_EDITOR_PALETTE;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		
		UITilePalette();
		
		void draw(float inter) const override;
		void update() override;
		
		void send_scroll(double yoffset) override;
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		
		void set_palette(const std::map<char, PaletteEntry>* palette);
		char get_selected_tile() const { return selected_tile; }
		
		// Add a custom tile to the palette
		void add_custom_tile(char symbol, int32_t id, const std::string& name);
		
		UIElement::Type get_type() const override { return TYPE; }
		
	protected:
		Button::State button_pressed(uint16_t buttonid) override;
		
	private:
		enum Buttons : uint16_t
		{
			BT_CLOSE,
			BT_ADD_TILE,
			BT_REMOVE_TILE
		};
		
		void update_layout();
		Point<int16_t> get_tile_position(size_t index) const;
		char get_tile_at_position(Point<int16_t> pos) const;
		
		const std::map<char, PaletteEntry>* current_palette;
		char selected_tile;
		int16_t scroll_offset;
		
		// UI layout constants
		static constexpr int16_t TILE_SIZE = 40;
		static constexpr int16_t TILE_SPACING = 5;
		static constexpr int16_t TILES_PER_ROW = 4;
		static constexpr int16_t PALETTE_WIDTH = TILES_PER_ROW * (TILE_SIZE + TILE_SPACING) + TILE_SPACING;
		static constexpr int16_t PALETTE_HEIGHT = 400;
		static constexpr int16_t HEADER_HEIGHT = 30;
		
		// Visual elements
		Text title;
		Text selected_info;
		Texture background;
		
		// Buttons - TODO: implement simple button for map editor
		// std::map<uint16_t, std::unique_ptr<Button>> buttons;
	};
}