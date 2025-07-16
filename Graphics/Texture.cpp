//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.							//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#include "Texture.h"

#include "GraphicsGL.h"

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

#include <iostream>

namespace ms
{
	Texture::Texture(nl::node src)
	{
		z_index = 0; // Default Z-index
		nl::node final_node = src;

		// --- NEW: Defensive Check for Unexpected Containers ---
		if (src && src.data_type() == nl::node::type::none && src.size() > 0) {
			// Container node received, searching for child bitmap 
			
			// Attempt to find a valid bitmap child, prioritizing "0"
			nl::node child_zero = src["0"];
			if (child_zero && child_zero.data_type() == nl::node::type::bitmap) {
				final_node = child_zero;
				// Using fallback child
			} else {
				for (const auto& child : src) {
					if (child.data_type() == nl::node::type::bitmap) {
						final_node = child;
						// Using child bitmap
						break;
					}
				}
			}
		}

		std::string node_name = final_node.name();
		bool is_hair = (node_name.find("hair") != std::string::npos || node_name.find("Hair") != std::string::npos);
		
		// Only log problematic cases to reduce spam
		
		if (final_node && final_node.data_type() == nl::node::type::bitmap)
		{
			try {
				origin = final_node["origin"];
			} catch (const std::exception& e) {
				origin = Point<int16_t>(0, 0); // Default origin
			}
			
			// Read Z-depth for proper layering with error handling
			try {
				// QUICK FIX: Force specific z-index values for character parts
				std::string node_name = final_node.name();
				if (node_name == "hair" || node_name == "backHair" || node_name.find("Hair") != std::string::npos) {
					z_index = 100; // Force hair to render on top
				} else if (node_name == "face") {
					z_index = 50; // Face rendered above body but below hair
				} else if (node_name == "body" || node_name == "arm" || node_name.find("Hand") != std::string::npos) {
					z_index = 25; // Body parts rendered at medium depth
				} else {
					// Check if z field exists and what type it is
					nl::node z_node = final_node["z"];
					if (z_node) {
						try {
							z_index = z_node.get_integer(0);
						} catch (const std::exception& e2) {
							z_index = 10; // Default fallback
						}
					} else {
						z_index = 10;
					}
				}
			} catch (const std::exception& e) {
				z_index = 5; // Default z-index
			}
			
			if (z_index == 0) {
				try {
					z_index = final_node["zM"].get_integer(0);
				} catch (const std::exception& e) {
					z_index = 0; // Keep default
				}
			}
			// Texture creation from node (debug output removed)

			if (final_node.root() == nl::nx::Map001)
			{
				const std::string& _outlink = final_node["_outlink"];

				if (!_outlink.empty())
				{
					size_t first = _outlink.find_first_of('/');

					if (first != std::string::npos)
					{
						const std::string& first_part = _outlink.substr(0, first);

						if (first_part == "Map")
						{
							const std::string& path = _outlink.substr(first + 1);
							nl::node foundOutlink = nl::nx::Map.resolve(path);

							if (foundOutlink)
								final_node = foundOutlink;
						}
					}
				}
			}

			// This is the critical conversion line, now performed on the correct node
			try {
				
				bitmap = final_node;
				
				// Bitmap assignment complete

				if (bitmap.id() == 0)
				{
				}

				dimensions = Point<int16_t>(bitmap.width(), bitmap.height());
				
				// Texture loaded successfully

				GraphicsGL::get().addbitmap(bitmap);
			} catch (const std::exception& e) {
				// Create invalid texture with default values
				dimensions = Point<int16_t>(0, 0);
			}
		}
		else
		{
		}
	}

	void Texture::draw(const DrawArgument& args) const
	{
		draw(args, Range<int16_t>(0, 0));
	}

	void Texture::draw(const DrawArgument& args, const Range<int16_t>& vertical) const
	{
		draw(args, vertical, Range<int16_t>(0, 0));
	}

	void Texture::draw(const DrawArgument& args, const Range<int16_t>& vertical, const Range<int16_t>& horizontal) const
	{
		if (!is_valid())
		{
			// Skipping draw: invalid texture
			return;
		}

		// Drawing texture 

		GraphicsGL::get().draw(
			bitmap,
			args.get_rectangle(origin, dimensions),
			vertical,
			horizontal,
			args.get_color(),
			args.get_angle()
		);
	}

	void Texture::shift(Point<int16_t> amount)
	{
		origin -= amount;
	}

	bool Texture::is_valid() const
	{
		return bitmap.id() > 0;
	}

	int16_t Texture::width() const
	{
		return dimensions.x();
	}

	int16_t Texture::height() const
	{
		return dimensions.y();
	}

	Point<int16_t> Texture::get_origin() const
	{
		return origin;
	}

	Point<int16_t> Texture::get_dimensions() const
	{
		return dimensions;
	}

	int Texture::get_z_index() const
	{
		return z_index;
	}
}