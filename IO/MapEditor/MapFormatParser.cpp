//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#include "MapFormatParser.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "../../Util/Misc.h"

namespace ms
{
	MapFormatParser::MapFormatParser()
	{
	}
	
	MapFormatParser::~MapFormatParser()
	{
	}
	
	std::unique_ptr<MapFormat> MapFormatParser::parse_file(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			error_message = "Failed to open file: " + filepath;
			return nullptr;
		}
		
		std::stringstream buffer;
		buffer << file.rdbuf();
		file.close();
		
		return parse_string(buffer.str());
	}
	
	std::unique_ptr<MapFormat> MapFormatParser::parse_string(const std::string& content)
	{
		auto map = std::make_unique<MapFormat>();
		
		// Parse each section
		if (!parse_metadata(content, *map))
			return nullptr;
			
		if (!parse_palette(content, *map))
			return nullptr;
			
		if (!parse_grid(content, *map))
			return nullptr;
			
		if (!parse_objects(content, *map))
			return nullptr;
		
		// Validate the parsed map
		if (!map->is_valid())
		{
			error_message = "Parsed map is invalid: " + map->get_validation_errors();
			return nullptr;
		}
		
		return map;
	}
	
	bool MapFormatParser::write_file(const MapFormat& map, const std::string& filepath)
	{
		std::string content = to_string(map);
		if (content.empty())
			return false;
			
		std::ofstream file(filepath);
		if (!file.is_open())
		{
			error_message = "Failed to create file: " + filepath;
			return false;
		}
		
		file << content;
		file.close();
		return true;
	}
	
	std::string MapFormatParser::to_string(const MapFormat& map)
	{
		std::stringstream output;
		
		output << write_metadata(map);
		output << "\n";
		output << write_palette(map);
		output << "\n";
		output << write_grid(map);
		output << "\n";
		output << write_objects(map);
		
		return output.str();
	}
	
	bool MapFormatParser::parse_metadata(const std::string& content, MapFormat& map)
	{
		// Extract map name
		size_t name_pos = content.find("name:");
		if (name_pos != std::string::npos)
		{
			size_t start = content.find_first_not_of(" \t", name_pos + 5);
			size_t end = content.find_first_of("\n\r", start);
			if (start != std::string::npos && end != std::string::npos)
			{
				std::string name = content.substr(start, end - start);
				// Remove quotes if present
				if (name.length() >= 2 && name.front() == '"' && name.back() == '"')
					name = name.substr(1, name.length() - 2);
				map.set_name(name);
			}
		}
		
		// Extract size
		size_t size_pos = content.find("size:");
		if (size_pos != std::string::npos)
		{
			size_t start = content.find('[', size_pos);
			size_t end = content.find(']', start);
			if (start != std::string::npos && end != std::string::npos)
			{
				std::string size_str = content.substr(start + 1, end - start - 1);
				auto parts = split(size_str, ',');
				if (parts.size() == 2)
				{
					int32_t width, height;
					if (parse_int(trim(parts[0]), width) && parse_int(trim(parts[1]), height))
					{
						map.set_size(static_cast<int16_t>(width), static_cast<int16_t>(height));
					}
				}
			}
		}
		
		// Extract return map
		size_t return_pos = content.find("return_map:");
		if (return_pos != std::string::npos)
		{
			size_t start = content.find_first_not_of(" \t", return_pos + 11);
			size_t end = content.find_first_of("\n\r", start);
			if (start != std::string::npos && end != std::string::npos)
			{
				int32_t return_map_id;
				if (parse_int(trim(content.substr(start, end - start)), return_map_id))
				{
					map.set_return_map(return_map_id);
				}
			}
		}
		
		return true;
	}
	
	bool MapFormatParser::parse_palette(const std::string& content, MapFormat& map)
	{
		std::string palette_section = extract_section(content, "palette:");
		if (palette_section.empty())
			return true; // Palette is optional
		
		std::istringstream stream(palette_section);
		std::string line;
		
		while (std::getline(stream, line))
		{
			// Skip empty lines
			line = trim(line);
			if (line.empty())
				continue;
			
			// Parse palette entry: "G : { id: 100450, name: "Grass_mid" }"
			size_t colon_pos = line.find(':');
			if (colon_pos != std::string::npos && colon_pos > 0)
			{
				char symbol = line[0];
				
				// Find the brackets
				size_t brace_start = line.find('{', colon_pos);
				size_t brace_end = line.find('}', brace_start);
				if (brace_start != std::string::npos && brace_end != std::string::npos)
				{
					std::string entry_content = line.substr(brace_start + 1, brace_end - brace_start - 1);
					
					// Parse id
					size_t id_pos = entry_content.find("id:");
					if (id_pos != std::string::npos)
					{
						size_t id_start = entry_content.find_first_not_of(" \t", id_pos + 3);
						size_t id_end = entry_content.find_first_of(",}", id_start);
						if (id_start != std::string::npos && id_end != std::string::npos)
						{
							int32_t id;
							if (parse_int(trim(entry_content.substr(id_start, id_end - id_start)), id))
							{
								// Parse name (optional)
								std::string name;
								size_t name_pos = entry_content.find("name:");
								if (name_pos != std::string::npos)
								{
									size_t quote_start = entry_content.find('"', name_pos);
									size_t quote_end = entry_content.find('"', quote_start + 1);
									if (quote_start != std::string::npos && quote_end != std::string::npos)
									{
										name = entry_content.substr(quote_start + 1, quote_end - quote_start - 1);
									}
								}
								
								map.add_palette_entry(symbol, PaletteEntry(id, name));
							}
						}
					}
				}
			}
		}
		
		return true;
	}
	
	bool MapFormatParser::parse_grid(const std::string& content, MapFormat& map)
	{
		std::string grid_section = extract_section(content, "grid:");
		if (grid_section.empty())
		{
			error_message = "Missing grid section";
			return false;
		}
		
		// Check if grid uses block notation (|)
		bool block_notation = grid_section.find('|') != std::string::npos;
		
		std::istringstream stream(grid_section);
		std::string line;
		int16_t y = 0;
		
		if (block_notation)
		{
			// Skip the first line with |
			std::getline(stream, line);
		}
		
		while (std::getline(stream, line) && y < map.get_height())
		{
			// Trim whitespace
			line = trim(line);
			if (line.empty())
				continue;
			
			// Process each character in the line
			for (int16_t x = 0; x < static_cast<int16_t>(line.length()) && x < map.get_width(); x++)
			{
				map.set_tile(x, y, line[x]);
			}
			y++;
		}
		
		return true;
	}
	
	bool MapFormatParser::parse_objects(const std::string& content, MapFormat& map)
	{
		std::string objects_section = extract_section(content, "objects:");
		if (objects_section.empty())
			return true; // Objects are optional
		
		// Parse player spawn
		size_t player_pos = objects_section.find("player_start:");
		if (player_pos != std::string::npos)
		{
			size_t bracket_start = objects_section.find('[', player_pos);
			size_t bracket_end = objects_section.find(']', bracket_start);
			if (bracket_start != std::string::npos && bracket_end != std::string::npos)
			{
				std::string coord_str = objects_section.substr(bracket_start + 1, bracket_end - bracket_start - 1);
				int16_t x, y;
				if (parse_coordinate(coord_str, x, y))
				{
					map.set_player_spawn(x, y);
				}
			}
		}
		
		// Parse portals
		size_t portals_pos = objects_section.find("portals:");
		if (portals_pos != std::string::npos)
		{
			size_t list_start = objects_section.find_first_of("\n", portals_pos);
			size_t list_end = objects_section.find_first_not_of(" \t\n\r-", list_start);
			
			std::string portals_str = objects_section.substr(list_start, list_end - list_start);
			std::istringstream portal_stream(portals_str);
			std::string portal_line;
			
			while (std::getline(portal_stream, portal_line))
			{
				portal_line = trim(portal_line);
				if (portal_line.empty() || portal_line[0] != '-')
					continue;
				
				// Parse portal entry
				PortalData portal;
				
				// Parse position
				size_t pos_start = portal_line.find("pos:");
				if (pos_start != std::string::npos)
				{
					size_t bracket_start = portal_line.find('[', pos_start);
					size_t bracket_end = portal_line.find(']', bracket_start);
					if (bracket_start != std::string::npos && bracket_end != std::string::npos)
					{
						std::string coord_str = portal_line.substr(bracket_start + 1, bracket_end - bracket_start - 1);
						parse_coordinate(coord_str, portal.x, portal.y);
					}
				}
				
				// Parse destination
				size_t dest_start = portal_line.find("dest:");
				if (dest_start != std::string::npos)
				{
					size_t quote_start = portal_line.find('"', dest_start);
					size_t quote_end = portal_line.find('"', quote_start + 1);
					if (quote_start != std::string::npos && quote_end != std::string::npos)
					{
						portal.destination_map = portal_line.substr(quote_start + 1, quote_end - quote_start - 1);
					}
				}
				
				map.add_portal(portal);
			}
		}
		
		// TODO: Parse NPCs and mob spawns when needed
		
		return true;
	}
	
	std::string MapFormatParser::extract_section(const std::string& content, const std::string& section_name)
	{
		size_t section_start = content.find(section_name);
		if (section_start == std::string::npos)
			return "";
		
		// Find the start of the section content
		size_t content_start = section_start + section_name.length();
		
		// Find the next section (look for next line that starts with a letter at column 0)
		size_t section_end = content.length();
		size_t pos = content.find('\n', content_start);
		
		while (pos != std::string::npos && pos < content.length() - 1)
		{
			// Check if next line starts with a non-whitespace character
			if (!std::isspace(content[pos + 1]) && content[pos + 1] != '-')
			{
				section_end = pos;
				break;
			}
			pos = content.find('\n', pos + 1);
		}
		
		return content.substr(content_start, section_end - content_start);
	}
	
	std::string MapFormatParser::trim(const std::string& str)
	{
		size_t first = str.find_first_not_of(" \t\n\r");
		if (first == std::string::npos)
			return "";
		size_t last = str.find_last_not_of(" \t\n\r");
		return str.substr(first, last - first + 1);
	}
	
	std::vector<std::string> MapFormatParser::split(const std::string& str, char delimiter)
	{
		std::vector<std::string> result;
		std::stringstream ss(str);
		std::string item;
		
		while (std::getline(ss, item, delimiter))
		{
			result.push_back(item);
		}
		
		return result;
	}
	
	bool MapFormatParser::parse_int(const std::string& str, int32_t& value)
	{
		try
		{
			value = std::stoi(str);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	
	bool MapFormatParser::parse_coordinate(const std::string& str, int16_t& x, int16_t& y)
	{
		auto parts = split(str, ',');
		if (parts.size() != 2)
			return false;
		
		int32_t x_val, y_val;
		if (parse_int(trim(parts[0]), x_val) && parse_int(trim(parts[1]), y_val))
		{
			x = static_cast<int16_t>(x_val);
			y = static_cast<int16_t>(y_val);
			return true;
		}
		
		return false;
	}
	
	std::string MapFormatParser::write_metadata(const MapFormat& map)
	{
		std::stringstream output;
		
		output << "# MapleStory Map Editor Format\n";
		output << "name: \"" << map.get_name() << "\"\n";
		output << "size: [" << map.get_width() << ", " << map.get_height() << "]\n";
		output << "return_map: " << map.get_return_map() << "\n";
		
		return output.str();
	}
	
	std::string MapFormatParser::write_palette(const MapFormat& map)
	{
		std::stringstream output;
		const auto& palette = map.get_palette();
		
		if (!palette.empty())
		{
			output << "palette:\n";
			for (const auto& entry : palette)
			{
				output << "  " << entry.first << " : { id: " << entry.second.id;
				if (!entry.second.name.empty())
				{
					output << ", name: \"" << entry.second.name << "\"";
				}
				output << " }\n";
			}
		}
		
		return output.str();
	}
	
	std::string MapFormatParser::write_grid(const MapFormat& map)
	{
		std::stringstream output;
		const auto& grid = map.get_grid();
		
		output << "grid: |\n";
		for (const auto& row : grid)
		{
			output << "  " << row << "\n";
		}
		
		return output.str();
	}
	
	std::string MapFormatParser::write_objects(const MapFormat& map)
	{
		std::stringstream output;
		
		output << "objects:\n";
		output << "  player_start: [" << map.get_player_spawn_x() << ", " << map.get_player_spawn_y() << "]\n";
		
		const auto& portals = map.get_portals();
		if (!portals.empty())
		{
			output << "  portals:\n";
			for (const auto& portal : portals)
			{
				output << "    - { pos: [" << portal.x << ", " << portal.y << "]";
				if (!portal.destination_map.empty())
				{
					output << ", dest: \"" << portal.destination_map << "\"";
				}
				else if (portal.destination_id > 0)
				{
					output << ", dest_id: " << portal.destination_id;
				}
				if (portal.type != "regular")
				{
					output << ", type: \"" << portal.type << "\"";
				}
				output << " }\n";
			}
		}
		
		const auto& npcs = map.get_npcs();
		if (!npcs.empty())
		{
			output << "  npcs:\n";
			for (const auto& npc : npcs)
			{
				output << "    - { id: " << npc.id << ", pos: [" << npc.x << ", " << npc.y << "]";
				if (npc.flip)
				{
					output << ", flip: true";
				}
				output << " }\n";
			}
		}
		
		const auto& mobs = map.get_mob_spawns();
		if (!mobs.empty())
		{
			output << "  mob_spawns:\n";
			for (const auto& mob : mobs)
			{
				output << "    - { id: " << mob.id << ", pos: [" << mob.x << ", " << mob.y << "]";
				if (mob.respawn_time != 30)
				{
					output << ", respawn: " << mob.respawn_time;
				}
				output << " }\n";
			}
		}
		
		return output.str();
	}
}