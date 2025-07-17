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
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nlnx/node.hpp>
#include <nlnx/nx.hpp>
#include "../Graphics/Texture.h"
#include "../Constants.h"
#include "../MapleStory.h"
#include <iostream>
#include <vector>
#include <string>

namespace ms
{
	// Centralized v83 UI asset compatibility layer
	class V83UIAssets
	{
	public:
		// Check if we're using v83/v87 assets
		static bool isV83Mode()
		{
			// Check for the existence of UIWindow2.img to determine version
			// v83/v87 uses UIWindow.img, newer versions use UIWindow2.img
			return !nl::nx::UI["UIWindow2.img"];
		}
		
		// Check if we're using v92 assets
		static bool isV92Mode()
		{
			// v92 has Title section with buttons, v83 doesn't have Title section
			nl::node login = nl::nx::UI["Login.img"];
			if (!login) {
				LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Login.img not found");
				return false;
			}
			
			nl::node title = login["Title"];
			if (!title) {
				LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Title section not found - not v92");
				return false;
			}
			
			// Check if Title section has any children (buttons)
			if (title.size() > 0) {
				LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Title section has " << title.size() << " children - v92 detected");
				return true;
			}
			
			// Alternative check: look for specific v92 buttons in Title
			// Sometimes the node exists but reports size 0, so check specific children
			if (title["BtLogin"] || title["BtQuit"] || title["BtHomePage"] || 
			    title["BtPasswdLost"] || title["BtEmailLost"] || title["BtEmailSave"]) {
				LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: Found v92 buttons in Title section");
				return true;
			}
			
			LOG(LOG_DEBUG, "[V83UIAssets] isV92Mode: No v92 indicators found");
			return false;
		}

		// === Helper function to resolve paths ===
		static nl::node resolvePath(const std::string& path)
		{
			std::vector<std::string> parts;
			std::string current;
			for (char c : path) {
				if (c == '/') {
					if (!current.empty()) {
						parts.push_back(current);
						current.clear();
					}
				} else {
					current += c;
				}
			}
			if (!current.empty()) {
				parts.push_back(current);
			}
			
			if (parts.empty()) return nl::node();
			
			nl::node node;
			if (parts[0] == "UI") {
				node = nl::nx::UI;
			} else if (parts[0] == "Map") {
				node = nl::nx::Map;
			} else if (parts[0] == "Map001") {
				node = nl::nx::Map001;
			} else {
				return nl::node();
			}
			
			for (size_t i = 1; i < parts.size() && node; ++i) {
				node = node[parts[i]];
			}
			
			return node;
		}

		// === Login Screen Assets ===
		static nl::node getLoginBackground()
		{
			// Based on actual NX structure analysis: Login background is in Common/frame
			static const std::vector<std::string> bgCandidates = {
				"UI/Login.img/Common/frame",         // Primary: Common frame (modern structure)
				"UI/Login.img/Common/selectWorld",   // Alternative: World select background
				"Map/Obj/login.img/back/0",          // v83 fallback
				"Map/Back/login/0",                  // v83 fallback
				"UI/Login.img/Title/effect"          // Last resort
			};
			
			for (const auto& path : bgCandidates) {
				nl::node n = resolvePath(path);
				if (n && n.data_type() == nl::node::type::bitmap) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found login background at: " << path);
					return n;
				}
			}
			
			LOG(LOG_ERROR, "[V83UIAssets] No login background found!");
			return nl::node();
		}

		static nl::node getLoginButton(const std::string& buttonName)
		{
			nl::node login = nl::nx::UI["Login.img"];
			if (!login) {
				LOG(LOG_ERROR, "[V83UIAssets] Login.img not found!");
				return nl::node();
			}
			
			// Based on actual NX structure analysis: correct button mappings
			std::vector<std::string> buttonPaths;
			
			if (buttonName == "Login") {
				if (isV92Mode()) {
					// v92 has login button in Title section
					buttonPaths = {
						"Title/BtLogin",       // v92 primary location
						"Common/BtStart",      // v83/v92 alternative
						"Common/BtStart2",     // v92 alternative
						"Common/BtOk",         // Fallback: OK button
						"BtOk"                 // Direct OK button
					};
				} else {
					// v83: No BtLogin exists - use BtStart from Common section
					buttonPaths = {
						"Common/BtStart",      // Primary: Login button is actually BtStart
						"Common/BtOk",         // Alternative: OK button
						"BtOk"                 // Direct OK button
					};
				}
			} else if (buttonName == "New") {
				if (isV92Mode()) {
					// v92: BtNew exists in both WorldSelect and CharSelect
					buttonPaths = {
						"CharSelect/BtNew",    // Primary for character creation
						"WorldSelect/BtNew",   // Alternative location
						"BtNew"                // Direct fallback
					};
				} else {
					// v83: BtNew exists only in CharSelect section
					buttonPaths = {
						"CharSelect/BtNew",    // Primary: New character button
						"BtNew"                // Fallback (won't exist)
					};
				}
			} else if (buttonName == "Quit") {
				if (isV92Mode()) {
					// v92 has quit button in Title section
					buttonPaths = {
						"Title/BtQuit",        // v92 primary location
						"Common/BtExit",       // v83/v92 alternative
						"BtCancel",            // Alternative: Cancel button
						"Common/BtCancel"      // Alternative: Cancel from common
					};
				} else {
					// v83: BtQuit doesn't exist - use BtExit from Common section
					buttonPaths = {
						"Common/BtExit",       // Primary: Exit button
						"BtCancel",            // Alternative: Cancel button
						"Common/BtCancel"      // Alternative: Cancel from common
					};
				}
			} else if (buttonName == "HomePage") {
				// v92 has these buttons in Title section
				buttonPaths = {
					"Title/BtHomePage",    // v92 location
					"Common/BtHomePage",   // v83 fallback
					"BtHomePage"           // Direct fallback
				};
			} else if (buttonName == "PasswdLost") {
				// v92 has these buttons in Title section
				buttonPaths = {
					"Title/BtPasswdLost",  // v92 location
					"Title/BtLoginIDLost", // v92 alternative
					"Common/BtPasswdLost", // v83 fallback
					"BtPasswdLost"         // Direct fallback
				};
			} else if (buttonName == "EmailLost") {
				// v92 has these buttons in Title section
				buttonPaths = {
					"Title/BtEmailLost",   // v92 location
					"Title/BtLoginIDLost", // v92 alternative
					"Common/BtEmailLost",  // v83 fallback
					"BtEmailLost"          // Direct fallback
				};
			} else if (buttonName == "EmailSave") {
				// v92 has these buttons in Title section
				buttonPaths = {
					"Title/BtEmailSave",   // v92 location
					"Title/BtLoginIDSave", // v92 alternative
					"Common/BtEmailSave",  // v83 fallback
					"BtEmailSave"          // Direct fallback
				};
			} else {
				// For other buttons, try basic patterns
				buttonPaths = {"Bt" + buttonName, buttonName, "Common/Bt" + buttonName, "Title/Bt" + buttonName};
			}
			
			for (const auto& path : buttonPaths) {
				nl::node btn;
				
				// Parse path and navigate to button
				if (path.find("Title/") == 0) {
					btn = login["Title"][path.substr(6)];
				} else if (path.find("Common/") == 0) {
					btn = login["Common"][path.substr(7)];
				} else if (path.find("CharSelect/") == 0) {
					btn = login["CharSelect"][path.substr(11)];
				} else if (path.find("WorldSelect/") == 0) {
					btn = login["WorldSelect"][path.substr(12)];
				} else {
					btn = login[path];
				}
				
				if (!btn) continue;
				
				// Modern button structure: Button/normal/0 or Button/disabled/0
				if (btn.data_type() == nl::node::type::none && btn.size() > 0) {
					// Try normal state first
					nl::node normal = btn["normal"];
					if (normal && normal.data_type() == nl::node::type::none && normal.size() > 0) {
						nl::node normalBitmap = normal["0"];
						if (normalBitmap && normalBitmap.data_type() == nl::node::type::bitmap) {
							LOG(LOG_DEBUG, "[V83UIAssets] Found button '" << buttonName << "' at '" << path << "/normal/0'");
							return btn;  // Return the button container
						}
					}
					
					// Try disabled state
					nl::node disabled = btn["disabled"];
					if (disabled && disabled.data_type() == nl::node::type::none && disabled.size() > 0) {
						nl::node disabledBitmap = disabled["0"];
						if (disabledBitmap && disabledBitmap.data_type() == nl::node::type::bitmap) {
							LOG(LOG_DEBUG, "[V83UIAssets] Found button '" << buttonName << "' at '" << path << "/disabled/0'");
							return btn;  // Return the button container
						}
					}
				}
				
				// Direct bitmap fallback
				if (btn.data_type() == nl::node::type::bitmap) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found button '" << buttonName << "' as direct bitmap at '" << path << "'");
					return btn;
				}
			}
			
			// If still not found, log what's available
			LOG(LOG_ERROR, "[V83UIAssets] Button '" << buttonName << "' not found. Searched paths:");
			for (const auto& path : buttonPaths) {
				LOG(LOG_ERROR, "  - " << path);
			}
			
			return nl::node();
		}

		// === World Select Assets ===
		static nl::node getWorldSelectBackground()
		{
			static const std::vector<std::string> bgCandidates = {
				"UI/Login.img/Common/selectWorld",     // Primary: Based on NX analysis
				"UI/Login.img/Common/frame",           // Alternative: Common frame
				"Map/Obj/login.img/WorldSelect/default/0",  // v83 fallback
				"Map/Back/worldselect/0",              // v83 fallback
				"UI/Login.img/WorldSelect/backgrnd"    // Modern fallback
			};
			
			for (const auto& path : bgCandidates) {
				nl::node n = resolvePath(path);
				if (n && n.data_type() == nl::node::type::bitmap) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found world select background at: " << path);
					return n;
				}
			}
			
			LOG(LOG_ERROR, "[V83UIAssets] No world select background found!");
			return nl::node();
		}

		static nl::node getWorldButton()
		{
			// Based on NX analysis: WorldSelect/BtWorld has numbered subfolders (0-22)
			// Return the container so the client can access individual world buttons
			nl::node worldSelect = nl::nx::UI["Login.img"]["WorldSelect"];
			if (!worldSelect) {
				LOG(LOG_ERROR, "[V83UIAssets] WorldSelect section not found!");
				return nl::node();
			}
			
			nl::node btWorld = worldSelect["BtWorld"];
			if (!btWorld) {
				LOG(LOG_ERROR, "[V83UIAssets] BtWorld not found in WorldSelect!");
				return nl::node();
			}
			
			LOG(LOG_DEBUG, "[V83UIAssets] Found BtWorld with " << btWorld.size() << " world buttons");
			return btWorld;
		}

		static nl::node getChannelButton(int channel)
		{
			if (isV92Mode())
			{
				// v92 might not have BtChannel at all
				nl::node worldSelect = nl::nx::UI["Login.img"]["WorldSelect"];
				
				// Try BtChannel first
				nl::node btChannel = worldSelect["BtChannel"];
				if (btChannel) {
					return btChannel[std::to_string(channel)];
				}
				
				// Fallback: use generic button from Common
				LOG(LOG_DEBUG, "[V83UIAssets] BtChannel not found for v92, using fallback button");
				return nl::nx::UI["Login.img"]["Common"]["BtOk"];
			}
			else if (isV83Mode())
			{
				// v83: channels might be in different structure
				return nl::nx::UI["Login.img"]["WorldSelect"]["BtChannel"][std::to_string(channel)];
			}
			else
			{
				// Modern: button:N format
				return nl::nx::UI["Login.img"]["WorldSelect"]["BtChannel"]["button:" + std::to_string(channel)];
			}
		}

		// === Character Select Assets ===
		static nl::node getCharSelectBackground()
		{
			// v92 doesn't have a specific charselect background in Login.img
			// Check alternative locations
			static const std::vector<std::string> bgCandidates = {
				"Map/Back/charselect.img/0",  // Check Map files
				"Map/Back/charselect/0",
				"Map001/Back/UI_login.img/1/0",
				"UI/Login.img/Common/frame",   // Use common frame as fallback
				"UI/Login.img/Common/selectWorld", // Or world select background
				"UI/Login.img/CharSelect/backgrnd",
				"UI/Login.img/CharSelect/bg",
				"UI/Login.img/CharSelect/background"
			};
			
			for (const auto& path : bgCandidates) {
				nl::node n = resolvePath(path);
				if (n && n.data_type() == nl::node::type::bitmap) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found character select background at: " << path);
					return n;
				}
			}
			
			// If no background found, use a default from Login.img
			nl::node commonFrame = nl::nx::UI["Login.img"]["Common"]["frame"];
			if (commonFrame && commonFrame.data_type() == nl::node::type::bitmap) {
				LOG(LOG_DEBUG, "[V83UIAssets] Using Common/frame as character select background fallback");
				return commonFrame;
			}
			
			LOG(LOG_ERROR, "[V83UIAssets] No character select background found!");
			return nl::node();
		}

		// === Inventory Window Assets ===
		static nl::node getInventoryBackground()
		{
			// Based on NX analysis: UIWindow.img/Item has multiple background options
			static const std::vector<std::string> bgCandidates = {
				"UI/UIWindow.img/Item/backgrnd",          // Primary background
				"UI/UIWindow.img/Item/productionBackgrnd", // Production background
				"UI/UIWindow2.img/Item/productionBackgrnd" // Modern fallback
			};
			
			for (const auto& path : bgCandidates) {
				nl::node n = resolvePath(path);
				if (n && n.data_type() == nl::node::type::bitmap) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found inventory background at: " << path);
					return n;
				}
			}
			
			LOG(LOG_ERROR, "[V83UIAssets] No inventory background found!");
			return nl::node();
		}

		static nl::node getInventoryTab(bool enabled, int tabIndex)
		{
			// Based on NX analysis: UIWindow.img/Item/Tab/[enabled|disabled]/[0-4]
			nl::node tabs = nl::nx::UI["UIWindow.img"]["Item"]["Tab"];
			if (!tabs) {
				LOG(LOG_ERROR, "[V83UIAssets] Item Tab section not found!");
				return nl::node();
			}
			
			std::string state = enabled ? "enabled" : "disabled";
			nl::node stateNode = tabs[state];
			if (!stateNode) {
				LOG(LOG_ERROR, "[V83UIAssets] Tab state '" << state << "' not found!");
				return nl::node();
			}
			
			nl::node tabNode = stateNode[std::to_string(tabIndex)];
			if (!tabNode) {
				LOG(LOG_ERROR, "[V83UIAssets] Tab " << tabIndex << " not found in state '" << state << "'!");
				return nl::node();
			}
			
			LOG(LOG_DEBUG, "[V83UIAssets] Found inventory tab " << tabIndex << " (" << state << ")");
			return tabNode;
		}

		// === Skills Window Assets ===
		static nl::node getSkillsBackground()
		{
			if (isV83Mode())
			{
				// v83 uses UIWindow.img
				return nl::nx::UI["UIWindow.img"]["Skill"]["backgrnd"];
			}
			else
			{
				// Modern uses UIWindow2.img
				return nl::nx::UI["UIWindow2.img"]["Skill"]["main"]["backgrnd"];
			}
		}

		static nl::node getSkillDisplay(int index)
		{
			if (isV83Mode())
			{
				// v83 might have simpler structure
				return nl::nx::UI["UIWindow.img"]["Skill"]["skill" + std::to_string(index)];
			}
			else
			{
				// Modern has main subfolder
				return nl::nx::UI["UIWindow2.img"]["Skill"]["main"]["skill" + std::to_string(index)];
			}
		}

		// === Common UI Elements ===
		static nl::node getCloseButton()
		{
			// Try Basic.img first - most common location
			nl::node btn = nl::nx::UI["Basic.img"]["BtClose"];
			if (btn) {
				LOG(LOG_DEBUG, "[V83UIAssets] Found close button at Basic.img/BtClose");
				return btn;
			}
			
			// Then check UIWindow.img variants
			if (isV92Mode())
			{
				// v92 uses different naming
				btn = nl::nx::UI["UIWindow.img"]["BtUIClose"];
				if (btn) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found close button at UIWindow.img/BtUIClose");
					return btn;
				}
				
				btn = nl::nx::UI["UIWindow.img"]["BtUIClose2"];
				if (btn) {
					LOG(LOG_DEBUG, "[V83UIAssets] Found close button at UIWindow.img/BtUIClose2");
					return btn;
				}
			}
			
			// Other variants
			btn = nl::nx::UI["Basic.img"]["BtClose3"];
			if (btn) return btn;
			
			// v83 might have it in different location
			btn = nl::nx::UI["UIWindow.img"]["BtClose"];
			if (btn) return btn;
			
			LOG(LOG_ERROR, "[V83UIAssets] No close button found!");
			return nl::node();
		}

		static nl::node getCommonButton(const std::string& buttonName)
		{
			// Try Login.img/Common first (v83 structure)
			nl::node btn = nl::nx::UI["Login.img"]["Common"][buttonName];
			if (btn) return btn;
			
			// Try Basic.img (alternative location)
			btn = nl::nx::UI["Basic.img"][buttonName];
			if (btn) return btn;
			
			// Modern location
			return nl::nx::UI["UIWindow2.img"]["Common"][buttonName];
		}

		// === Character Creation Assets ===
		static nl::node getRaceSelectButton(const std::string& race, int state)
		{
			// v83 might have different race selection structure
			if (isV83Mode())
			{
				// v83 only has Explorer, no Cygnus/Aran
				if (race != "normal") return nl::node();
				
				return nl::nx::UI["Login.img"]["NewChar"][std::to_string(state)];
			}
			else
			{
				// Modern has RaceSelect with multiple classes
				return nl::nx::UI["Login.img"]["RaceSelect"][race][std::to_string(state)];
			}
		}

		// === Status Bar Assets ===  
		static nl::node getStatusBarBackground()
		{
			if (isV83Mode())
			{
				// v83 uses simpler structure
				return nl::nx::UI["StatusBar.img"]["base"]["backgrnd"];
			}
			else
			{
				// Modern uses StatusBar2.img
				return nl::nx::UI["StatusBar2.img"]["mainBar"]["backgrnd"];
			}
		}

		// === Chat Window Assets ===
		static nl::node getChatBackground()
		{
			if (isV83Mode())
			{
				// v83 uses UIWindow.img
				return nl::nx::UI["UIWindow.img"]["Chat"]["backgrnd"];
			}
			else
			{
				// Modern uses separate ChatBalloon.img
				return nl::nx::UI["ChatBalloon.img"]["backgrnd"];
			}
		}

		// === Minimap Assets ===
		static nl::node getMinimapBackground()
		{
			if (isV83Mode())
			{
				// v83 uses UIWindow.img
				return nl::nx::UI["UIWindow.img"]["MiniMap"]["backgrnd"];
			}
			else
			{
				// Modern uses UIWindow2.img
				return nl::nx::UI["UIWindow2.img"]["MiniMap"]["backgrnd"];
			}
		}

		// === Helper function to safely get texture ===
		// NOTE: For Sprite creation, use the node directly, not this function
		// Sprites accept nl::node in their constructor, not Texture
		static Texture getTexture(nl::node node, const std::string& debugName = "")
		{
			if (!node)
			{
				LOG(LOG_ERROR, "[V83UIAssets] Node not found for: " << debugName);
				return Texture();
			}

			// Handle container nodes with bitmap children (hybrid nodes)
			if (node.data_type() == nl::node::type::none && node.size() > 0)
			{
				// Try child "0" first
				nl::node child = node["0"];
				if (child && child.data_type() == nl::node::type::bitmap)
				{
					return Texture(child);
				}

				// Try any bitmap child
				for (const auto& c : node)
				{
					if (c.data_type() == nl::node::type::bitmap)
					{
						return Texture(c);
					}
				}
			}

			// Direct bitmap node
			if (node.data_type() == nl::node::type::bitmap)
			{
				return Texture(node);
			}

			LOG(LOG_ERROR, "[V83UIAssets] Failed to create texture from node: " << debugName << " (type=" << (int)node.data_type() << ")");
			return Texture();
		}
	};
}