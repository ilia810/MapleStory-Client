//////////////////////////////////////////////////////////////////////////////////
//	LegacyUI - Helper functions for v83/v87 UI implementation
//////////////////////////////////////////////////////////////////////////////////
#include "LegacyUI.h"
#include "Misc.h"
#include "../Configuration.h"
#include "../Graphics/Text.h"
#include "../Graphics/Color.h"

#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace ms
{
	namespace LegacyUI
	{
		// Track warned nodes to avoid spam
		static std::unordered_set<std::string> warned_nodes;
		
		Texture get_or_dummy(nl::node node, const std::string& debug_path)
		{
			if (node && node.data_type() == nl::node::type::bitmap)
			{
				return Texture(node);
			}
			
			// Log warning for missing node
			if (!debug_path.empty())
			{
				warn_missing_node(debug_path);
			}
			
			// Return invalid texture
			return Texture();
		}
		
		std::unique_ptr<MapleButton> make_simple3(nl::node node, Point<int16_t> position)
		{
			if (!node)
			{
				return nullptr;
			}
			
			// Try to create MapleButton - it will handle missing states gracefully
			try
			{
				return std::make_unique<MapleButton>(node, position);
			}
			catch (...)
			{
				warn_missing_node("Button node (make_simple3): " + node.name());
				return nullptr;
			}
		}
		
		Point<int16_t> calc_anchor(Point<int16_t> desired_center, const Texture& texture)
		{
			if (texture.is_valid())
			{
				return desired_center - texture.get_origin();
			}
			return desired_center;
		}
		
		void play_sound_safe(Sound::Name sound_id)
		{
			try
			{
				Sound(sound_id).play();
			}
			catch (...)
			{
				// Silently ignore missing sounds
			}
		}
		
		void draw_fallback_rect(Point<int16_t> position, Point<int16_t> dimensions, Color::Name color)
		{
			// TODO: Implement fallback rectangle drawing for missing assets
			// This would require access to the graphics context
			// For now, just log that we need a fallback
		}
		
		void warn_missing_node(const std::string& node_path)
		{
			// Only warn once per unique path
			if (warned_nodes.find(node_path) == warned_nodes.end())
			{
				warned_nodes.insert(node_path);
			}
		}
		
		std::string get_version_text()
		{
			return Configuration::get().get_version();
		}
		
		Text create_version_text()
		{
			std::string version_text = get_version_text();
			return Text(Text::Font::A12B, Text::Alignment::LEFT, Color::Name::LEMONGRASS, "Ver. " + version_text);
		}
		
		bool is_valid_character_name(const std::string& name)
		{
			// Basic validation for v83/v87
			if (name.length() < 4 || name.length() > 12)
				return false;
			
			// Check for valid characters (alphanumeric)
			for (char c : name)
			{
				if (!std::isalnum(c))
					return false;
			}
			
			// Check against forbidden names
			try
			{
				nl::node ForbiddenName = nl::nx::Etc["ForbiddenName.img"];
				if (ForbiddenName)
				{
					std::string lName = name;
					std::transform(lName.begin(), lName.end(), lName.begin(), ::tolower);
					
					for (auto forbiddenNode : ForbiddenName)
					{
						std::string forbidden = forbiddenNode.get_string();
						std::string fName = forbidden;
						std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);
						
						if (lName.find(fName) != std::string::npos)
							return false;
					}
				}
			}
			catch (...)
			{
				// If forbidden name check fails, just continue with basic validation
			}
			
			return true;
		}
		
		Text create_input_text(const std::string& initial_text)
		{
			return Text(Text::Font::A12M, Text::Alignment::LEFT, Color::Name::WHITE, initial_text);
		}
		
		Text create_stat_text(int32_t value)
		{
			return Text(Text::Font::A11M, Text::Alignment::RIGHT, Color::Name::WHITE, std::to_string(value));
		}
	}
}