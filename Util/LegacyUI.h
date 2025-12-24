//////////////////////////////////////////////////////////////////////////////////
//	LegacyUI - Helper functions for v83/v87 UI implementation
//	Centralizes layout constants and provides safe UI asset loading
//	Also provides offset corrections for v92 asset origin mismatches
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../Graphics/Texture.h"
#include "../Graphics/Sprite.h"
#include "../Graphics/Text.h"
#include "../Graphics/Color.h"
#include "../IO/Components/MapleButton.h"
#include "../Audio/Audio.h"
#include "../Template/Point.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include <memory>
#include <string>
#include <unordered_map>

namespace ms
{
	namespace LegacyUI
	{
		//////////////////////////////////////////////////////////////////////////////
		// OFFSET CORRECTION SYSTEM
		// Compensates for origin differences between v83 and v92 assets
		// When v92 assets have different origins than v83, UI elements appear
		// shifted. These corrections are applied at render time.
		//////////////////////////////////////////////////////////////////////////////
		namespace OffsetCorrection
		{
			// Get offset correction for a specific UI asset
			// Returns the delta to apply: render_pos = code_pos + delta
			Point<int16_t> get_offset(const std::string& asset_path);

			// Initialize offset correction tables (called once at startup)
			void initialize();

			// Check if offset corrections are available
			bool is_initialized();

			// Apply offset correction to a position
			inline Point<int16_t> apply(const std::string& asset_path, Point<int16_t> pos)
			{
				return pos + get_offset(asset_path);
			}

			// Known offsets for common UI elements (v92 vs v83 delta)
			// Positive values shift right/down, negative shift left/up
			namespace Offsets
			{
				// StatusBar offsets
				static constexpr Point<int16_t> STATUSBAR_HP = Point<int16_t>(0, 0);
				static constexpr Point<int16_t> STATUSBAR_MP = Point<int16_t>(0, 0);
				static constexpr Point<int16_t> STATUSBAR_EXP = Point<int16_t>(0, 0);

				// Inventory offsets
				static constexpr Point<int16_t> INVENTORY_SLOT = Point<int16_t>(0, 0);
				static constexpr Point<int16_t> INVENTORY_TAB = Point<int16_t>(0, 0);

				// Login screen offsets
				static constexpr Point<int16_t> LOGIN_BUTTON = Point<int16_t>(0, 0);
				static constexpr Point<int16_t> LOGIN_BACKGROUND = Point<int16_t>(0, 0);
			}
		}

		//////////////////////////////////////////////////////////////////////////////
		// DYNAMIC POSITIONING
		// Provides resolution-independent positioning for UI elements
		//////////////////////////////////////////////////////////////////////////////
		namespace DynamicPos
		{
			// Anchor types for UI positioning
			enum class Anchor
			{
				TOP_LEFT,
				TOP_CENTER,
				TOP_RIGHT,
				CENTER_LEFT,
				CENTER,
				CENTER_RIGHT,
				BOTTOM_LEFT,
				BOTTOM_CENTER,
				BOTTOM_RIGHT
			};

			// Calculate position based on anchor and offset from anchor point
			Point<int16_t> calculate(Anchor anchor, Point<int16_t> offset, int16_t screen_width, int16_t screen_height);

			// Get position relative to screen edge with padding
			inline Point<int16_t> from_right(int16_t padding, int16_t y, int16_t screen_width)
			{
				return Point<int16_t>(screen_width - padding, y);
			}

			inline Point<int16_t> from_bottom(int16_t x, int16_t padding, int16_t screen_height)
			{
				return Point<int16_t>(x, screen_height - padding);
			}

			inline Point<int16_t> from_bottom_right(int16_t x_padding, int16_t y_padding, int16_t screen_width, int16_t screen_height)
			{
				return Point<int16_t>(screen_width - x_padding, screen_height - y_padding);
			}
		}

		// Layout constants for v83/v87 character creation screens
		namespace Layout
		{
			// Screen dimensions (base resolution)
			static constexpr int16_t SCREEN_WIDTH = 800;
			static constexpr int16_t SCREEN_HEIGHT = 600;
			
			// Race Selection (already implemented)
			namespace RaceSelect
			{
				static constexpr Point<int16_t> EXPLORER_POS(200, 300);
				static constexpr Point<int16_t> KNIGHT_POS(400, 300);
				static constexpr Point<int16_t> ARAN_POS(600, 300);
			}
			
			// Explorer Creation
			namespace ExplorerCreation
			{
				static constexpr Point<int16_t> NAME_INPUT_POS(350, 100);
				static constexpr Point<int16_t> STAT_DICE_POS(500, 200);
				static constexpr Point<int16_t> OK_BUTTON_POS(300, 500);
				static constexpr Point<int16_t> CANCEL_BUTTON_POS(450, 500);
				static constexpr Point<int16_t> JOB_BANNER_POS(100, 150);
				
				// Stat display positions
				static constexpr Point<int16_t> STR_POS(400, 250);
				static constexpr Point<int16_t> DEX_POS(400, 280);
				static constexpr Point<int16_t> INT_POS(400, 310);
				static constexpr Point<int16_t> LUK_POS(400, 340);
			}
			
			// Knight Creation  
			namespace KnightCreation
			{
				static constexpr Point<int16_t> NAME_INPUT_POS(350, 100);
				static constexpr Point<int16_t> GENDER_MALE_POS(250, 200);
				static constexpr Point<int16_t> GENDER_FEMALE_POS(350, 200);
				static constexpr Point<int16_t> HAIR_DICE_POS(500, 250);
				static constexpr Point<int16_t> OK_BUTTON_POS(300, 500);
				static constexpr Point<int16_t> CANCEL_BUTTON_POS(450, 500);
			}
			
			// Aran Creation
			namespace AranCreation
			{
				static constexpr Point<int16_t> NAME_INPUT_POS(350, 100);
				static constexpr Point<int16_t> GENDER_MALE_POS(250, 200);
				static constexpr Point<int16_t> GENDER_FEMALE_POS(350, 200);
				static constexpr Point<int16_t> HAIR_DICE_POS(500, 250);
				static constexpr Point<int16_t> OK_BUTTON_POS(300, 500);
				static constexpr Point<int16_t> CANCEL_BUTTON_POS(450, 500);
			}
		}
		
		// Sound IDs for legacy UI
		namespace LegacySound
		{
			static constexpr Sound::Name BUTTON_OVER = Sound::Name::BUTTONOVER;
			static constexpr Sound::Name BUTTON_CLICK = Sound::Name::BUTTONCLICK;
			static constexpr Sound::Name DICE_ROLL = Sound::Name::SCROLLUP; // Fallback
			static constexpr Sound::Name UI_OPEN = Sound::Name::RACESELECT;
			static constexpr Sound::Name UI_CLOSE = Sound::Name::SCROLLUP;
		}
		
		// Safe asset loading functions
		
		/// <summary>
		/// Safely loads a texture from a node, returns invalid texture if node doesn't exist
		/// Logs a warning once per missing node to help with debugging
		/// </summary>
		Texture get_or_dummy(nl::node node, const std::string& debug_path = "");
		
		/// <summary>
		/// Creates a simple 3-state button (normal, hover, pressed) from a node
		/// Returns nullptr if node is invalid
		/// </summary>
		std::unique_ptr<MapleButton> make_simple3(nl::node node, Point<int16_t> position = Point<int16_t>(0, 0));
		
		/// <summary>
		/// Calculates proper position by subtracting texture origin from desired center point
		/// </summary>
		Point<int16_t> calc_anchor(Point<int16_t> desired_center, const Texture& texture);
		
		/// <summary>
		/// Safely plays a sound, does nothing if sound is invalid
		/// </summary>
		void play_sound_safe(Sound::Name sound_id);
		
		/// <summary>
		/// Draws a fallback colored rectangle when texture is missing (for QA/debugging)
		/// </summary>
		void draw_fallback_rect(Point<int16_t> position, Point<int16_t> dimensions, Color::Name color);
		
		/// <summary>
		/// Logs a missing UI node warning (only once per unique path)
		/// </summary>
		void warn_missing_node(const std::string& node_path);
		
		/// <summary>
		/// Gets version text for display
		/// </summary>
		std::string get_version_text();
		
		/// <summary>
		/// Creates standard version text display
		/// </summary>
		Text create_version_text();
		
		/// <summary>
		/// Validates character name according to v83/v87 rules
		/// </summary>
		bool is_valid_character_name(const std::string& name);
		
		/// <summary>
		/// Helper to create text input field with standard styling
		/// </summary>
		Text create_input_text(const std::string& initial_text = "");
		
		/// <summary>
		/// Helper to create stat display text
		/// </summary>
		Text create_stat_text(int32_t value);
	}
}