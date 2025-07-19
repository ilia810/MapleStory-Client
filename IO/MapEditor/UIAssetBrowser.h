//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor						//
//	Copyright (C) 2024												//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../../Graphics/Text.h"
#include "../../Graphics/Texture.h"
#include <vector>
#include <functional>
#include <string>

namespace ms
{
	class UIAssetBrowser : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::MAP_EDITOR_ASSET_BROWSER;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = true;
		
		UIAssetBrowser();
		
		void draw(float inter) const override;
		void update() override;
		
		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		UIElement::Type get_type() const override { return TYPE; }
		
		// Asset management
		void refresh_assets();
		void set_filter(const std::string& filter);
		void set_asset_selection_callback(std::function<void(int32_t)> callback);
		
		// Get currently selected asset
		int32_t get_selected_asset() const { return selected_asset_id; }
		
	protected:
		Button::State button_pressed(uint16_t buttonid) override;
		
	private:
		struct AssetEntry
		{
			int32_t id;
			std::string name;
			std::string category;
			Texture texture;
			Point<int16_t> position;
			Point<int16_t> size;
			bool visible;
			
			AssetEntry(int32_t id, const std::string& name, const std::string& category) :
				id(id), name(name), category(category), 
				position(0, 0), size(64, 64), visible(true) {}
		};
		
		void init_layout();
		void update_visible_assets();
		void draw_asset_entry(const AssetEntry& asset, float alpha) const;
		int32_t get_asset_at(Point<int16_t> pos) const;
		void load_asset_textures();
		
		std::vector<AssetEntry> assets;
		std::vector<AssetEntry*> visible_assets;
		std::string current_filter;
		int32_t selected_asset_id;
		int32_t hovered_asset_id;
		
		// Scroll state
		int32_t scroll_offset;
		int32_t max_scroll;
		int32_t assets_per_row;
		int32_t visible_rows;
		
		// Visual elements
		Text title_text;
		Text filter_text;
		Color bg_color;
		Color selection_color;
		Color hover_color;
		
		// Callback
		std::function<void(int32_t)> on_asset_selected;
	};
}