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
#include "NxFiles.h"
#include "AssetRegistry.h"
#include <iostream>
#include <fstream>

#ifdef USE_NX
#include <fstream>
#include <cstring>

#include <nlnx/node.hpp>
#include <nlnx/nx.hpp>
#include <nlnx/file.hpp>

namespace ms
{
	namespace NxFiles
	{
		Error init()
		{
			// Check for required files, but be more flexible with optional v87-missing files
			constexpr std::array<const char*, 8> required_files = {
				"Base.nx", "Character.nx", "Effect.nx", "Etc.nx",
				"Item.nx", "Map.nx", "UI.nx", "String.nx"
			};

			// Check required files
			for (auto filename : required_files)
				if (std::ifstream{ filename }.good() == false)
					return Error(Error::Code::MISSING_FILE, filename);

			// Check optional files and log warnings for missing ones
			for (auto filename : filenames)
			{
				if (std::ifstream{ filename }.good() == false)
				{
					// Skip if it's a required file (already checked above)
					bool is_required = false;
					for (auto required : required_files)
						if (std::strcmp(filename, required) == 0)
						{
							is_required = true;
							break;
						}
					if (!is_required)
					{
						// Log warning for missing optional file (would need logging system)
						// For now, continue without error
					}
				}
			}

			// Loading NX files
			try
			{
				nl::nx::load_all();
				
				// Test Character.nx Hair directory immediately after loading
				nl::node character_node = nl::nx::Character;
				if (character_node) {
					nl::node hair_dir = character_node["Hair"];
					if (hair_dir) {
						nl::node hair_30030 = hair_dir["00030030.img"];
						if (hair_30030) {
							// Hair 00030030.img (admin character) found in NX
						} else {
							// Hair 00030030.img (admin character) NOT found in NX
						}
					} else {
						// Hair directory NOT found in Character.nx
					}
				} else {
					// Character.nx failed to load
				}
				
				// Verifying file access
				
				// Simple test to check if load_all actually loaded anything
				// Basic NX validation complete
				
				// Initialize the AssetRegistry after NX files are loaded
				// Initializing AssetRegistry
				AssetRegistry& registry = AssetRegistry::get();
				if (registry.load())
				{
					// AssetRegistry ready
				}
				else
				{
					// AssetRegistry initialization failed
				}
				
				// NX Structure exploration for debugging
				// Exploring NX structure
				
				// Check if comprehensive extraction is requested
				char* extract_all = nullptr;
				size_t len = 0;
				errno_t err = _dupenv_s(&extract_all, &len, "EXTRACT_ALL_NX");
				bool full_extraction = (err == 0 && extract_all && std::string(extract_all) == "1");
				if (extract_all) free(extract_all);
				
				// Disable extraction for testing login screen
				full_extraction = false;
				
				if (full_extraction) {
					// FULL NX EXTRACTION MODE - Creating detailed structure files...
					
					// Create smart extraction function - focused and size-controlled
					auto extract_to_file = [](const nl::node& node, const std::string& nx_name, const std::string& path, int depth, std::ofstream& file, int& total_nodes) -> void {
						auto extract_recursive = [&](const nl::node& n, const std::string& p, int d, auto& self) -> void {
							// Smart depth limiting based on file type
							int max_depth = 4; // Default depth
							if (nx_name == "Map" || nx_name == "Character") max_depth = 3; // Large files
							else if (nx_name == "UI" || nx_name == "Item") max_depth = 5; // Important for asset registry
							else if (nx_name == "String") max_depth = 6; // Text data is compact
							
							if (d > max_depth) return;
							
							std::string indent(d * 2, ' ');
							std::string node_type;
							total_nodes++;
							
							// Determine node type with icons
							if (n.data_type() == nl::node::type::bitmap) node_type = "🖼️";
							else if (n.data_type() == nl::node::type::audio) node_type = "🔊";
							else if (n.data_type() == nl::node::type::string) node_type = "📝";
							else if (n.data_type() == nl::node::type::integer) node_type = "🔢";
							else if (n.data_type() == nl::node::type::real) node_type = "💯";
							else if (n.data_type() == nl::node::type::vector) node_type = "📍";
							else if (n.size() > 0) node_type = "📁";
							else node_type = "📄";
							
							file << indent << "├─ " << n.name() << " " << node_type;
							
							// Show child count for folders
							if (n.size() > 0) {
								file << " (" << n.size() << " items)";
							}
							
							// Add preview data for important leaf nodes
							if (n.size() == 0 && d <= 3) {
								try {
									if (n.data_type() == nl::node::type::string) {
										std::string str_val = n.get_string();
										if (str_val.length() < 30) {
											file << " = \"" << str_val << "\"";
										} else {
											file << " = \"" << str_val.substr(0, 27) << "...\"";
										}
									} else if (n.data_type() == nl::node::type::integer) {
										file << " = " << n.get_integer();
									} else if (n.data_type() == nl::node::type::vector) {
										auto vec = n.get_vector();
										file << " = (" << vec.first << ", " << vec.second << ")";
									}
								} catch (...) {
									// Ignore data extraction errors
								}
							}
							
							file << std::endl;
							
							// Smart child limiting based on depth and importance
							int child_limit = 50; // Default
							if (d == 0) child_limit = 20; // Root level - show overview
							else if (d == 1) child_limit = 30; // Second level - show structure
							else if (d >= 3) child_limit = 15; // Deep levels - be selective
							
							// Special handling for important UI nodes
							bool is_important = (nx_name == "UI" && (
								n.name().find("Login") != std::string::npos ||
								n.name().find("Title") != std::string::npos ||
								n.name().find("Button") != std::string::npos ||
								n.name().find("Bt") != std::string::npos
							));
							
							if (is_important) child_limit = 100; // Show more for important UI
							
							// Explore children with smart limiting
							int child_count = 0;
							int important_count = 0;
							
							for (const auto& child : n) {
								// Always show important items
								bool child_important = (child.name().find("Bt") != std::string::npos ||
													   child.name().find("Login") != std::string::npos ||
													   child.name().find("Title") != std::string::npos ||
													   child.name().find("backgrnd") != std::string::npos ||
													   child.name().find("frame") != std::string::npos);
								
								if (child_important) {
									important_count++;
									self(child, p + "/" + n.name(), d + 1, self);
								} else if (child_count < child_limit) {
									child_count++;
									self(child, p + "/" + n.name(), d + 1, self);
								} else if (child_count == child_limit) {
									file << indent << "  └─ ... (" << (n.size() - child_count - important_count) << " more items)" << std::endl;
									break;
								}
							}
						};
						
						extract_recursive(node, path, depth, extract_recursive);
					};
					
					// Create output directory
					system("mkdir nx_structures 2>nul");
					
					// Extract all major NX files
					std::vector<std::pair<std::string, const nl::node*>> nx_files = {
						{"UI", &nl::nx::UI},
						{"Map", &nl::nx::Map},
						{"Character", &nl::nx::Character},
						{"Item", &nl::nx::Item},
						{"Skill", &nl::nx::Skill},
						{"Effect", &nl::nx::Effect},
						{"Sound", &nl::nx::Sound},
						{"String", &nl::nx::String},
						{"Etc", &nl::nx::Etc},
						{"Base", &nl::nx::Base},
						{"Mob", &nl::nx::Mob},
						{"Npc", &nl::nx::Npc},
						{"Quest", &nl::nx::Quest},
						{"Reactor", &nl::nx::Reactor}
					};
					
					// Check for split files
					if (!nl::nx::Map001.name().empty()) {
						nx_files.push_back({"Map001", &nl::nx::Map001});
					}
					if (!nl::nx::Map002.name().empty()) {
						nx_files.push_back({"Map002", &nl::nx::Map002});
					}
					if (!nl::nx::Sound2.name().empty()) {
						nx_files.push_back({"Sound2", &nl::nx::Sound2});
					}
					
					// Extract each file
					for (const auto& nx_pair : nx_files) {
						const std::string& name = nx_pair.first;
						const nl::node& nx_node = *nx_pair.second;
						
						if (nx_node.size() == 0) {
							// Skipping empty/not loaded nx file
							continue;
						}
						
						std::string filename = "nx_structures/" + name + "_current.txt";
						std::ofstream file(filename);
						
						if (!file.is_open()) {
							// Failed to create file
							continue;
						}
						
						file << "🗂️ NX STRUCTURE ANALYSIS: " << name << ".nx" << std::endl;
						file << "📅 Generated: Current HeavenMS v83 Client Session" << std::endl;
						file << "🎯 Purpose: AssetRegistry Development & Asset Management" << std::endl;
						file << "================================================================================" << std::endl;
						file << "📊 Overview:" << std::endl;
						file << "   📦 Root children: " << nx_node.size() << std::endl;
						file << "   🏗️ Structure optimized for manageable file size" << std::endl;
						file << "   ⭐ Important assets prioritized (Login, UI, Buttons)" << std::endl;
						file << std::endl;
						
						int total_nodes = 0;
						extract_to_file(nx_node, name, name, 0, file, total_nodes);
						
						file << std::endl;
						file << "================================================================================" << std::endl;
						file << "Total nodes: " << total_nodes << std::endl;
						file << "================================================================================" << std::endl;
						
						file.close();
						// Extracted nx file structure
					}
					
					// Create enhanced summary file
					std::ofstream summary("nx_structures/📋_NX_CURRENT_SUMMARY.txt");
					summary << "🎮 HEAVENMS CLIENT - CURRENT NX STRUCTURE ANALYSIS" << std::endl;
					summary << "📅 Generated from live v83 nx files" << std::endl;
					summary << "🎯 Optimized for AssetRegistry development" << std::endl;
					summary << "================================================================================" << std::endl;
					summary << std::endl;
					
					summary << "📁 AVAILABLE NX FILES:" << std::endl;
					int total_available = 0;
					for (const auto& nx_pair : nx_files) {
						const nl::node& node = *nx_pair.second;
						if (node.size() > 0) {
							summary << "  ✅ " << nx_pair.first << "_current.txt - " << node.size() << " root items" << std::endl;
							total_available++;
						} else {
							summary << "  ❌ " << nx_pair.first << ".nx - NOT LOADED" << std::endl;
						}
					}
					
					summary << std::endl;
					summary << "🎯 KEY ASSETREGISTRY MAPPINGS:" << std::endl;
					summary << "  🔐 Login Buttons: UI.nx/Login.img/Title/Bt*" << std::endl;
					summary << "  🖼️ Login Background: UI.nx/Login.img/Notice/Loading/backgrnd" << std::endl;
					summary << "  👤 Character Select: UI.nx/Login.img/CharSelect/*" << std::endl;
					summary << "  🌍 World Select: UI.nx/Login.img/WorldSelect/*" << std::endl;
					summary << "  🗺️ Map Backgrounds: Map.nx/Back/*" << std::endl;
					summary << "  💰 Items: Item.nx/*" << std::endl;
					summary << "  👥 Characters: Character.nx/*" << std::endl;
					summary << "  ✨ Effects: Effect.nx/*" << std::endl;
					summary << std::endl;
					
					summary << "📋 STRUCTURE NOTES:" << std::endl;
					summary << "  📦 Architecture: v83 consolidated format" << std::endl;
					summary << "  🚫 No split files: Map001.nx/Map002.nx not used" << std::endl;
					summary << "  🏗️ UI consolidation: All login assets in UI.nx" << std::endl;
					summary << "  🎨 Button hierarchy: Title/* (not Title_new/*)" << std::endl;
					summary << "  📏 File sizes: Optimized for readability" << std::endl;
					summary << "  ⭐ Priority: Important assets (Login/UI/Buttons) emphasized" << std::endl;
					summary << std::endl;
					
					summary << "💡 USAGE TIPS:" << std::endl;
					summary << "  1. Each *_current.txt shows manageable subset of nx file" << std::endl;
					summary << "  2. Important assets (Login, Buttons) always included" << std::endl;
					summary << "  3. Use for AssetRegistry path development" << std::endl;
					summary << "  4. Icons show data types: 🖼️=bitmap 📁=folder 📝=string etc." << std::endl;
					summary << std::endl;
					
					summary << "📊 STATISTICS:" << std::endl;
					summary << "  🗂️ NX files documented: " << total_available << std::endl;
					summary << "  📅 Session: Current live extraction" << std::endl;
					
					summary.close();
					
					// Complete NX extraction finished
					// All structures saved to nx_structures/ directory
					// Check COMPLETE_NX_SUMMARY.txt for overview
				}
				
				// NX structure exploration complete
			}
			catch (const std::exception& ex)
			{
				static const std::string message = ex.what();

				return Error(Error::Code::NLNX, message.c_str());
			}

			// Removed strict UI version check for v87 compatibility
			// The previous check for "Login.img/WorldSelect/BtChannel/layer:bg"
			// would fail on v87 data which uses different UI structure
			
			return Error::Code::NONE;
		}
	}
}
#endif