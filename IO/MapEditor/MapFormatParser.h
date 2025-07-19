//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the MapleStory Map Editor							//
//	Copyright (C) 2024															//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "MapFormat.h"
#include <string>
#include <memory>

namespace ms
{
	class MapFormatParser
	{
	public:
		MapFormatParser();
		~MapFormatParser();
		
		// Parse a map file and return a MapFormat object
		std::unique_ptr<MapFormat> parse_file(const std::string& filepath);
		
		// Parse map data from a string
		std::unique_ptr<MapFormat> parse_string(const std::string& content);
		
		// Write a MapFormat to a file
		bool write_file(const MapFormat& map, const std::string& filepath);
		
		// Convert a MapFormat to string representation
		std::string to_string(const MapFormat& map);
		
		// Get the last error message
		const std::string& get_error() const { return error_message; }
		
	private:
		// Parse individual sections
		bool parse_metadata(const std::string& content, MapFormat& map);
		bool parse_palette(const std::string& content, MapFormat& map);
		bool parse_grid(const std::string& content, MapFormat& map);
		bool parse_objects(const std::string& content, MapFormat& map);
		
		// Helper functions
		std::string extract_section(const std::string& content, const std::string& section_name);
		std::string trim(const std::string& str);
		std::vector<std::string> split(const std::string& str, char delimiter);
		bool parse_int(const std::string& str, int32_t& value);
		bool parse_coordinate(const std::string& str, int16_t& x, int16_t& y);
		
		// Write helpers
		std::string write_metadata(const MapFormat& map);
		std::string write_palette(const MapFormat& map);
		std::string write_grid(const MapFormat& map);
		std::string write_objects(const MapFormat& map);
		
		std::string error_message;
	};
}