//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor						//
//	Copyright (C) 2024												//
//////////////////////////////////////////////////////////////////////////////////
#include "UIAssetBrowser.h"
#include "../../Graphics/GraphicsGL.h"
#include "../UI.h"
#include "../../Template/Rectangle.h"
#include "../../MapleStory.h"
#include "../../Util/NxFiles.h"
#include <iostream>

namespace ms
{
	UIAssetBrowser::UIAssetBrowser() :
		current_filter(""),
		selected_asset_id(-1),
		hovered_asset_id(-1),
		scroll_offset(0),
		max_scroll(0),
		assets_per_row(4),
		visible_rows(6),
		title_text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::BLACK),
		filter_text(Text::Font::A11M, Text::Alignment::LEFT, Color::Name::DARKGREY),
		bg_color(static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(240), static_cast<uint8_t>(255)),
		selection_color(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(255), static_cast<uint8_t>(128)),
		hover_color(static_cast<uint8_t>(200), static_cast<uint8_t>(220), static_cast<uint8_t>(255), static_cast<uint8_t>(128))
	{
		// Position asset browser on the right side
		position = Point<int16_t>(600, 60);
		dimension = Point<int16_t>(300, 400);
		active = true;
		
		title_text.change_text("Asset Browser");
		filter_text.change_text("Filter: (none)");
		
		init_layout();
		refresh_assets();
	}
	
	void UIAssetBrowser::init_layout()
	{
		// Calculate layout parameters
		int16_t content_width = dimension.x() - 20; // 10px padding on each side
		int16_t asset_size = (content_width - (assets_per_row - 1) * 5) / assets_per_row; // 5px spacing between assets
		
		// Update asset entry size
		for (auto& asset : assets)
		{
			asset.size = Point<int16_t>(asset_size, asset_size);
		}
		
		update_visible_assets();
	}
	
	void UIAssetBrowser::refresh_assets()
	{
		assets.clear();
		
		LOG(LOG_INFO, "[AssetBrowser] Loading map tile assets");
		
		// Load common MapleStory tile assets
		// For now, we'll create some example entries - in a real implementation,
		// you would scan the NX files for available tiles
		
		// Grass tiles
		assets.emplace_back(100450, "Grass_Mid", "Nature");
		assets.emplace_back(100451, "Grass_Left", "Nature");
		assets.emplace_back(100452, "Grass_Right", "Nature");
		assets.emplace_back(100453, "Grass_Corner_TL", "Nature");
		assets.emplace_back(100454, "Grass_Corner_TR", "Nature");
		
		// Stone tiles
		assets.emplace_back(100500, "Stone_Mid", "Stone");
		assets.emplace_back(100501, "Stone_Left", "Stone");
		assets.emplace_back(100502, "Stone_Right", "Stone");
		assets.emplace_back(100503, "Stone_Corner_TL", "Stone");
		assets.emplace_back(100504, "Stone_Corner_TR", "Stone");
		
		// Water tiles
		assets.emplace_back(100600, "Water_Surface", "Water");
		assets.emplace_back(100601, "Water_Deep", "Water");
		assets.emplace_back(100602, "Water_Shallow", "Water");
		
		// Wood/Platform tiles
		assets.emplace_back(100700, "Wood_Platform", "Platform");
		assets.emplace_back(100701, "Wood_Plank", "Platform");
		assets.emplace_back(100702, "Wood_Support", "Platform");
		
		// Dirt tiles
		assets.emplace_back(100800, "Dirt_Surface", "Ground");
		assets.emplace_back(100801, "Dirt_Underground", "Ground");
		assets.emplace_back(100802, "Dirt_Rocky", "Ground");
		
		LOG(LOG_INFO, "[AssetBrowser] Loaded " << assets.size() << " assets");
		
		// TODO: Load actual textures from NX files
		// load_asset_textures();
		
		update_visible_assets();
	}
	
	void UIAssetBrowser::update_visible_assets()
	{
		visible_assets.clear();
		
		// Filter assets based on current filter
		for (auto& asset : assets)
		{
			bool matches_filter = current_filter.empty() || 
								 asset.name.find(current_filter) != std::string::npos ||
								 asset.category.find(current_filter) != std::string::npos;
			
			asset.visible = matches_filter;
			if (matches_filter)
			{
				visible_assets.push_back(&asset);
			}
		}
		
		// Calculate positions for visible assets
		int16_t start_x = position.x() + 10;
		int16_t start_y = position.y() + 50; // Leave space for title
		int16_t current_x = start_x;
		int16_t current_y = start_y;
		int16_t asset_size = visible_assets.empty() ? 64 : visible_assets[0]->size.x();
		int16_t spacing = 5;
		
		int32_t col = 0;
		for (auto* asset : visible_assets)
		{
			asset->position = Point<int16_t>(current_x, current_y);
			
			col++;
			if (col >= assets_per_row)
			{
				col = 0;
				current_x = start_x;
				current_y += asset_size + spacing;
			}
			else
			{
				current_x += asset_size + spacing;
			}
		}
		
		// Calculate max scroll based on content height
		int32_t total_rows = (static_cast<int32_t>(visible_assets.size()) + assets_per_row - 1) / assets_per_row;
		int32_t content_height = total_rows * (asset_size + spacing);
		int32_t available_height = dimension.y() - 60; // Title space
		max_scroll = std::max(0, content_height - available_height);
	}
	
	void UIAssetBrowser::set_filter(const std::string& filter)
	{
		current_filter = filter;
		filter_text.change_text("Filter: " + (filter.empty() ? "(none)" : filter));
		update_visible_assets();
	}
	
	void UIAssetBrowser::set_asset_selection_callback(std::function<void(int32_t)> callback)
	{
		on_asset_selected = callback;
	}
	
	void UIAssetBrowser::draw(float inter) const
	{
		// Draw background
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
		Color border_color(static_cast<uint8_t>(120), static_cast<uint8_t>(120), static_cast<uint8_t>(120), static_cast<uint8_t>(255));
		// Top
		GraphicsGL::get().drawrectangle(position.x(), position.y(), dimension.x(), 2, border_color.r(), border_color.g(), border_color.b(), border_color.a());
		// Bottom
		GraphicsGL::get().drawrectangle(position.x(), position.y() + dimension.y() - 2, dimension.x(), 2, border_color.r(), border_color.g(), border_color.b(), border_color.a());
		// Left
		GraphicsGL::get().drawrectangle(position.x(), position.y(), 2, dimension.y(), border_color.r(), border_color.g(), border_color.b(), border_color.a());
		// Right
		GraphicsGL::get().drawrectangle(position.x() + dimension.x() - 2, position.y(), 2, dimension.y(), border_color.r(), border_color.g(), border_color.b(), border_color.a());
		
		// Draw title
		title_text.draw(DrawArgument(position + Point<int16_t>(10, 15)));
		
		// Draw filter text
		filter_text.draw(DrawArgument(position + Point<int16_t>(10, 35)));
		
		// Draw visible assets
		Rectangle<int16_t> clip_rect(
			position.x() + 2,
			position.x() + dimension.x() - 2,
			position.y() + 50,
			position.y() + dimension.y() - 2
		);
		
		for (const auto* asset : visible_assets)
		{
			Point<int16_t> asset_pos = asset->position - Point<int16_t>(0, scroll_offset);
			
			// Only draw if visible in clipping rectangle
			if (asset_pos.y() + asset->size.y() >= clip_rect.top() && 
				asset_pos.y() <= clip_rect.bottom())
			{
				draw_asset_entry(*asset, 1.0f);
			}
		}
	}
	
	void UIAssetBrowser::draw_asset_entry(const AssetEntry& asset, float alpha) const
	{
		Point<int16_t> asset_pos = asset.position - Point<int16_t>(0, scroll_offset);
		
		// Draw selection/hover background
		if (asset.id == selected_asset_id)
		{
			GraphicsGL::get().drawrectangle(
				asset_pos.x() - 2,
				asset_pos.y() - 2,
				asset.size.x() + 4,
				asset.size.y() + 4,
				selection_color.r(),
				selection_color.g(),
				selection_color.b(),
				static_cast<uint8_t>(selection_color.a() * alpha)
			);
		}
		else if (asset.id == hovered_asset_id)
		{
			GraphicsGL::get().drawrectangle(
				asset_pos.x() - 2,
				asset_pos.y() - 2,
				asset.size.x() + 4,
				asset.size.y() + 4,
				hover_color.r(),
				hover_color.g(),
				hover_color.b(),
				static_cast<uint8_t>(hover_color.a() * alpha)
			);
		}
		
		// Draw asset placeholder (colored rectangle for now)
		// TODO: Draw actual texture when asset loading is implemented
		Color asset_color;
		if (asset.category == "Nature")
			asset_color = Color(static_cast<uint8_t>(100), static_cast<uint8_t>(200), static_cast<uint8_t>(100), static_cast<uint8_t>(255));
		else if (asset.category == "Stone")
			asset_color = Color(static_cast<uint8_t>(150), static_cast<uint8_t>(150), static_cast<uint8_t>(150), static_cast<uint8_t>(255));
		else if (asset.category == "Water")
			asset_color = Color(static_cast<uint8_t>(100), static_cast<uint8_t>(150), static_cast<uint8_t>(255), static_cast<uint8_t>(255));
		else if (asset.category == "Platform")
			asset_color = Color(static_cast<uint8_t>(139), static_cast<uint8_t>(69), static_cast<uint8_t>(19), static_cast<uint8_t>(255));
		else
			asset_color = Color(static_cast<uint8_t>(101), static_cast<uint8_t>(67), static_cast<uint8_t>(33), static_cast<uint8_t>(255));
		
		GraphicsGL::get().drawrectangle(
			asset_pos.x(),
			asset_pos.y(),
			asset.size.x(),
			asset.size.y(),
			asset_color.r(),
			asset_color.g(),
			asset_color.b(),
			static_cast<uint8_t>(asset_color.a() * alpha)
		);
		
		// Draw asset border
		Color asset_border(static_cast<uint8_t>(80), static_cast<uint8_t>(80), static_cast<uint8_t>(80), static_cast<uint8_t>(255));
		// Top
		GraphicsGL::get().drawrectangle(asset_pos.x(), asset_pos.y(), asset.size.x(), 1, asset_border.r(), asset_border.g(), asset_border.b(), static_cast<uint8_t>(asset_border.a() * alpha));
		// Bottom
		GraphicsGL::get().drawrectangle(asset_pos.x(), asset_pos.y() + asset.size.y() - 1, asset.size.x(), 1, asset_border.r(), asset_border.g(), asset_border.b(), static_cast<uint8_t>(asset_border.a() * alpha));
		// Left
		GraphicsGL::get().drawrectangle(asset_pos.x(), asset_pos.y(), 1, asset.size.y(), asset_border.r(), asset_border.g(), asset_border.b(), static_cast<uint8_t>(asset_border.a() * alpha));
		// Right
		GraphicsGL::get().drawrectangle(asset_pos.x() + asset.size.x() - 1, asset_pos.y(), 1, asset.size.y(), asset_border.r(), asset_border.g(), asset_border.b(), static_cast<uint8_t>(asset_border.a() * alpha));
		
		// Draw asset ID text (for debugging)
		Text id_text(Text::Font::A11M, Text::Alignment::CENTER, Color::Name::BLACK);
		id_text.change_text(std::to_string(asset.id));
		id_text.draw(DrawArgument(asset_pos + Point<int16_t>(asset.size.x() / 2, asset.size.y() / 2)));
	}
	
	void UIAssetBrowser::update()
	{
		// Update any animations or states
	}
	
	Cursor::State UIAssetBrowser::send_cursor(bool clicked, Point<int16_t> cursorpos)
	{
		// Check if cursor is over asset browser
		Rectangle<int16_t> browser_bounds(position.x(), position.x() + dimension.x(), position.y(), position.y() + dimension.y());
		if (!browser_bounds.contains(cursorpos))
		{
			hovered_asset_id = -1;
			return UIElement::send_cursor(clicked, cursorpos);
		}
		
		// Find which asset is hovered
		int32_t prev_hover = hovered_asset_id;
		hovered_asset_id = get_asset_at(cursorpos);
		
		if (clicked && hovered_asset_id != -1)
		{
			selected_asset_id = hovered_asset_id;
			if (on_asset_selected)
			{
				on_asset_selected(selected_asset_id);
			}
			LOG(LOG_INFO, "[AssetBrowser] Selected asset: " << selected_asset_id);
			return Cursor::State::CLICKING;
		}
		
		return (hovered_asset_id != -1) ? Cursor::State::CANCLICK : Cursor::State::IDLE;
	}
	
	int32_t UIAssetBrowser::get_asset_at(Point<int16_t> pos) const
	{
		Point<int16_t> relative_pos = pos - position;
		
		// Check if in content area
		if (relative_pos.y() < 50 || relative_pos.y() >= dimension.y() - 2)
			return -1;
		
		for (const auto* asset : visible_assets)
		{
			Point<int16_t> asset_pos = asset->position - position - Point<int16_t>(0, scroll_offset);
			
			Rectangle<int16_t> asset_bounds(
				asset_pos.x(),
				asset_pos.x() + asset->size.x(),
				asset_pos.y(),
				asset_pos.y() + asset->size.y()
			);
			
			if (asset_bounds.contains(relative_pos))
			{
				return asset->id;
			}
		}
		
		return -1;
	}
	
	Button::State UIAssetBrowser::button_pressed(uint16_t buttonid)
	{
		return Button::State::NORMAL;
	}
	
	void UIAssetBrowser::load_asset_textures()
	{
		// TODO: Implement actual texture loading from NX files
		// This would scan through Map.nx or other relevant files to find tile textures
		// For now, we use colored rectangles as placeholders
		LOG(LOG_INFO, "[AssetBrowser] Asset texture loading not yet implemented");
	}
}