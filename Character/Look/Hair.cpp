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
#include "Hair.h"

#include <iostream>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
	Hair::Hair(int32_t hairid, const BodyDrawInfo& drawinfo)
	{
		try {
			std::string hairpath = "000" + std::to_string(hairid) + ".img";
			nl::node character_node = nl::nx::Character;
			if (!character_node) {
				throw std::runtime_error("Character node not available");
			}
			nl::node hair_dir = character_node["Hair"];
			if (!hair_dir) {
				throw std::runtime_error("Hair directory not found");
			}
			
			nl::node hairnode = nl::nx::Character["Hair"]["000" + std::to_string(hairid) + ".img"];
			if (!hairnode) {
				// Try fallback to a default hair (30000 - basic black hair)
				if (hairid != 30000) {
					hairnode = nl::nx::Character["Hair"]["00030000.img"];
				}
				
				if (!hairnode) {
					name = "Missing Hair";
					color = "None";
					return;
				}
			}

		for (auto stance_iter : Stance::names)
		{
			Stance::Id stance = stance_iter.first;
			const std::string& stancename = stance_iter.second;

			nl::node stancenode = hairnode[stancename];

			if (!stancenode) {
				continue;
			}

			for (uint8_t frame = 0; nl::node framenode = stancenode[frame]; ++frame)
			{
				for (nl::node layernode : framenode)
				{
					std::string layername = layernode.name();
					auto layer_iter = layers_by_name.find(layername);

					if (layer_iter == layers_by_name.end())
					{
						LOG(LOG_DEBUG, "Unknown Hair::Layer name: [" << layername << "]\tLocation: [" << hairnode.name() << "][" << stancename << "][" << frame << "]");
						continue;
					}

					Layer layer = layer_iter->second;
					Point<int16_t> brow;
					try {
						brow = layernode["map"]["brow"];
					} catch (const std::exception& e) {
						brow = Point<int16_t>(0, 0); // Use default
					}
					Point<int16_t> shift = drawinfo.gethairpos(stance, frame) - brow;

					bool texture_valid = false;
					try {
						texture_valid = Texture(layernode).is_valid();
					} catch (const std::exception& e) {
						texture_valid = false;
					}
					
					if (texture_valid)
					{
						try {
							stances[stance][layer]
								.emplace(frame, layernode)
								.first->second.shift(shift);
						} catch (const std::exception& e) {
							// Error during texture emplace/shift
						}
						continue;
					}

					std::string defaultstancename = "default";

					if (layername.substr(0, 4) == "back")
						defaultstancename = "backDefault";
					nl::node defaultnode = hairnode[defaultstancename][layername];

					try {
						if (Texture(defaultnode).is_valid())
						{
							stances[stance][layer]
								.emplace(frame, defaultnode)
								.first->second.shift(shift);
							continue;
						}
					} catch (const std::exception& e) {
						// Error with default texture
					}

					nl::node defaultnode2 = defaultnode["0"];

					try {
						if (Texture(defaultnode2).is_valid())
						{
							stances[stance][layer]
								.emplace(frame, defaultnode2)
								.first->second.shift(shift);
							continue;
						}
					} catch (const std::exception& e) {
						// Error with default texture [0]
					}

					LOG(LOG_DEBUG, "Invalid Hair::Layer texture\tName: [" << layername << "]\tLocation: [" << hairnode.name() << "][" << stancename << "][" << frame << "]");
				}
			}
		}

		try {
			name = nl::nx::String["Eqp.img"]["Eqp"]["Hair"][std::to_string(hairid)]["name"];
		} catch (const std::exception& e) {
			name = "Hair " + std::to_string(hairid); // Use fallback name
		}

		constexpr size_t NUM_COLORS = 8;

		constexpr const char* haircolors[NUM_COLORS] =
		{
			"Black", "Red", "Orange", "Blonde", "Green", "Blue", "Violet", "Brown"
		};

		size_t index = hairid % 10;
		color = (index < NUM_COLORS) ? haircolors[index] : "";
		
		} catch (const std::exception& e) {
			// Don't re-throw, create empty hair instead
			name = "Error Hair";
			color = "Error";
			return;
		}
	}

	void Hair::draw(Layer layer, Stance::Id stance, uint8_t frame, const DrawArgument& args) const
	{
		
		auto frameit = stances[stance][layer].find(frame);

		if (frameit == stances[stance][layer].end()) {
			return;
		}

		// Removed spam logging
		
		frameit->second.draw(args);
	}

	const std::string& Hair::get_name() const
	{
		return name;
	}

	const std::string& Hair::getcolor() const
	{
		return color;
	}

	const std::unordered_map<std::string, Hair::Layer> Hair::layers_by_name =
	{
		{ "hair",					Hair::Layer::DEFAULT		},
		{ "hairBelowBody",			Hair::Layer::BELOWBODY		},
		{ "hairOverHead",			Hair::Layer::OVERHEAD		},
		{ "hairShade",				Hair::Layer::SHADE			},
		{ "backHair",				Hair::Layer::BACK			},
		{ "backHairBelowCap",		Hair::Layer::BELOWCAP		},
		{ "backHairBelowCapNarrow",	Hair::Layer::BELOWCAPNARROW },
		{ "backHairBelowCapWide",	Hair::Layer::BELOWCAPWIDE	}
	};
}