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
#include "AssetRegistry.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef USE_NX
#include <nlnx/nx.hpp>
#endif

namespace ms
{
    // Static member definitions
    const std::string AssetRegistry::empty_string_ = "";
    const std::vector<std::string> AssetRegistry::empty_vector_ = {};

    AssetRegistry& AssetRegistry::get()
    {
        static AssetRegistry instance;
        return instance;
    }

    bool AssetRegistry::load(const std::string& map_filepath)
    {
        last_error_ = ErrorCode::NONE;
        last_error_message_.clear();

        // Initialize the name to ID mapping
        initializeNameMapping();

        // For now, use hardcoded mappings instead of YAML parsing
        // This provides immediate functionality while YAML support can be added later
        initializeHardcodedMappings();

        loaded_ = true;
        return true;
    }

    bool AssetRegistry::is_loaded() const
    {
        return loaded_;
    }

    const std::string& AssetRegistry::getPath(AssetID id) const
    {
        if (!loaded_)
        {
            setError(ErrorCode::NX_NOT_LOADED, "AssetRegistry not loaded");
            return empty_string_;
        }

        auto it = asset_map_.find(id);
        if (it == asset_map_.end())
        {
            setError(ErrorCode::ASSET_NOT_FOUND, "Asset ID not found in registry");
            return empty_string_;
        }

        const auto& info = it->second;
        
        // Try each path in priority order until we find a valid one
        for (const auto& path : info.prioritized_paths)
        {
#ifdef USE_NX
            if (isPathValid(path))
            {
                return path;
            }
#else
            // Without NX, return the first path
            return path;
#endif
        }

        // No valid paths found
        setError(ErrorCode::NO_VALID_PATHS, "No valid paths found for asset");
        return empty_string_;
    }

    const std::vector<std::string>& AssetRegistry::getAllPaths(AssetID id) const
    {
        auto it = asset_map_.find(id);
        if (it == asset_map_.end())
        {
            return empty_vector_;
        }
        return it->second.prioritized_paths;
    }

    bool AssetRegistry::hasValidPath(AssetID id) const
    {
        if (!loaded_)
            return false;

        auto it = asset_map_.find(id);
        if (it == asset_map_.end())
            return false;

#ifdef USE_NX
        for (const auto& path : it->second.prioritized_paths)
        {
            if (isPathValid(path))
                return true;
        }
        return false;
#else
        return !it->second.prioritized_paths.empty();
#endif
    }

    const std::string& AssetRegistry::getAssetType(AssetID id) const
    {
        auto it = asset_map_.find(id);
        if (it == asset_map_.end())
            return empty_string_;
        return it->second.asset_type;
    }

    const std::string& AssetRegistry::getAssetNotes(AssetID id) const
    {
        auto it = asset_map_.find(id);
        if (it == asset_map_.end())
            return empty_string_;
        return it->second.notes;
    }

    std::string AssetRegistry::resolveFullPath(AssetID id) const
    {
        const std::string& path = getPath(id);
        if (path.empty())
            return "";

        // Path format: "nx_file:path/to/asset"
        auto colon_pos = path.find(':');
        if (colon_pos == std::string::npos)
            return path;

        return path.substr(colon_pos + 1);
    }

#ifdef USE_NX

    bool AssetRegistry::isPathValid(const std::string& path) const
    {
        if (path.empty())
            return false;

        try
        {
            nl::node node = resolvePathToNode(path);
            return !node.name().empty();
        }
        catch (...)
        {
            return false;
        }
    }

    nl::node AssetRegistry::resolvePathToNode(const std::string& full_path) const
    {
        // Parse path format: "nx_file:path/to/asset"
        auto colon_pos = full_path.find(':');
        if (colon_pos == std::string::npos)
        {
            // No colon, assume it's just a path for UI.nx
            // No colon found, treating as UI path
            return nl::nx::UI.resolve(full_path);
        }

        std::string nx_file = full_path.substr(0, colon_pos);
        std::string asset_path = full_path.substr(colon_pos + 1);
        
        // Parsed nx file and asset path

        // Map nx file name to actual node (v83 consolidated - no Map001/Map002)
        if (nx_file == "UI") {
            // Resolving UI path
            nl::node result = nl::nx::UI.resolve(asset_path);
            // UI path resolved
            return result;
        }
        else if (nx_file == "Map")
            return nl::nx::Map.resolve(asset_path);
        else if (nx_file == "Character")
            return nl::nx::Character.resolve(asset_path);
        else if (nx_file == "Sound")
            return nl::nx::Sound.resolve(asset_path);
        else if (nx_file == "String")
            return nl::nx::String.resolve(asset_path);
        else if (nx_file == "Item")
            return nl::nx::Item.resolve(asset_path);
        else if (nx_file == "Effect")
            return nl::nx::Effect.resolve(asset_path);
        else if (nx_file == "Etc")
            return nl::nx::Etc.resolve(asset_path);

        // Unknown nx file
        return nl::node();
    }

    nl::node AssetRegistry::resolve(AssetID id) const
    {
        const std::string& path = getPath(id);
        if (path.empty()) {
            // Failed to get path for AssetID
            setError(ErrorCode::ASSET_NOT_FOUND, "No path configured for AssetID.");
            return nl::node();
        }

        // Resolving AssetID to path
        nl::node result = resolvePathToNode(path);

        if (result.name().empty()) {
            // Path resolution failed
            setError(ErrorCode::NO_VALID_PATHS, "Path resolution failed for: " + path);
            return result;
        }

        // --- NEW: Intelligent Hybrid Node Resolution Logic ---
        auto asset_map_it = asset_map_.find(id);
        if (asset_map_it == asset_map_.end()) {
            // This case should ideally not happen if getPath succeeded, but is a safe check.
            return result;
        }
        const auto& assetInfo = asset_map_it->second;

        // Only attempt to traverse if the node is a container AND the asset allows hybrid resolution
        if (result.data_type() == nl::node::type::none && result.size() > 0 && assetInfo.allow_hybrid_resolve) {
            // Hybrid container detected, searching for child bitmap
            
            // Prioritize child named "0" as this is a common pattern, then iterate.
            nl::node child_zero = result["0"];
            if (child_zero && child_zero.data_type() == nl::node::type::bitmap) {
                // Found valid child bitmap
                return child_zero;
            }

            // Fallback to iterating all children if "0" is not a valid bitmap.
            for (const auto& child : result) {
                if (child.data_type() == nl::node::type::bitmap) {
                    // Found valid child bitmap
                    return child;
                }
            }

            setError(ErrorCode::NO_VALID_PATHS, "Hybrid node '" + path + "' contains no valid child bitmaps.");
            // Hybrid node contains no valid child bitmaps
            return nl::node(); // Return invalid node if no suitable child is found.
        }
        
        // Asset successfully resolved
        return result;
    }
#endif

    AssetRegistry::ErrorCode AssetRegistry::getLastError() const
    {
        return last_error_;
    }

    const std::string& AssetRegistry::getLastErrorMessage() const
    {
        return last_error_message_;
    }

    void AssetRegistry::setError(ErrorCode code, const std::string& message) const
    {
        last_error_ = code;
        last_error_message_ = message;
    }

    void AssetRegistry::initializeNameMapping()
    {
        // Map string names to AssetID enum values for future YAML parsing
        name_to_id_["UI_Login_Background"] = AssetID::UI_Login_Background;
        name_to_id_["UI_Login_Title_Background"] = AssetID::UI_Login_Title_Background;
        name_to_id_["UI_Login_Version_Position"] = AssetID::UI_Login_Version_Position;
        name_to_id_["UI_Shop_Background"] = AssetID::UI_Shop_Background;
        name_to_id_["Map_Login_Background"] = AssetID::Map_Login_Background;
        name_to_id_["Sound_UI_Button_Click"] = AssetID::Sound_UI_Button_Click;
        // Add more mappings as needed
    }

    void AssetRegistry::initializeHardcodedMappings()
    {
        // Helper function to create AssetInfo with default hybrid resolve = false
        auto createAssetInfo = [](const std::vector<std::string>& paths, const std::string& type, 
                                 const std::string& notes, const std::vector<std::string>& consumers,
                                 const std::string& version, const std::vector<std::string>& fallback,
                                 bool hybrid = false) -> AssetInfo {
            AssetInfo info;
            info.prioritized_paths = paths;
            info.asset_type = type;
            info.notes = notes;
            info.consumers = consumers;
            info.version_compatibility = version;
            info.fallback_paths = fallback;
            info.allow_hybrid_resolve = hybrid;
            return info;
        };

        // Temporary hardcoded mappings based on extracted asset data
        // These replace the most commonly used hardcoded paths
        
        // UI Login Assets - UPDATED FOR V92 STRUCTURE WITH HYBRID HANDLING
        asset_map_[AssetID::UI_Login_Background] = createAssetInfo(
            {"UI:Login.img/Title/effect"}, "Texture",
            "Login screen background (full-screen animation)",
            {"IO/UITypes/UILogin.cpp"}, "v92", {}, true);

        asset_map_[AssetID::UI_Login_Title_Background] = createAssetInfo(
            {"UI:Login.img/Title/effect"}, "Texture",
            "Login title-area background (same as main background)",
            {"IO/UITypes/UILogin.cpp"}, "v92", {}, true);

        asset_map_[AssetID::UI_Login_Version_Position] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Point",
            "Version text position - using frame as reference",
            {"IO/UITypes/UILogin.cpp"}, "v83", {});

        // Shop Assets
        asset_map_[AssetID::UI_Shop_Background] = createAssetInfo(
            {"UI:UIWindow2.img/Shop2/backgrnd"}, "Texture",
            "Shop interface background texture",
            {"IO/UITypes/UIShop.cpp"}, "all", {});

        // Sound Assets
        asset_map_[AssetID::Sound_UI_Button_Click] = createAssetInfo(
            {"Sound:UI.img/BtMouseClick"}, "Audio",
            "Standard UI button click sound effect",
            {"Audio/Audio.cpp"}, "all", {});

        // Character Select Background - CORRECTED for v83 consolidated structure
        asset_map_[AssetID::UI_CharSelect_Background] = createAssetInfo(
            {"UI:Login.img/Notice/Loading/backgrnd/0"}, "Node",
            "Character selection background - using available background",
            {"IO/UITypes/UICharSelect.cpp"}, "v83", {});

        // Login UI Elements - Essential for login screen functionality
        asset_map_[AssetID::UI_Login_Capslock_Warning] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Caps lock warning indicator", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Checkbox_Unchecked] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture", 
            "Save login unchecked checkbox", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Checkbox_Checked] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Save login checked checkbox", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Button_Login] = createAssetInfo(
            {"UI:Login.img/BtLogin/normal/0"}, "Texture",
            "Login button", {"IO/UITypes/UILogin.cpp"}, "v92", {}, true);

        asset_map_[AssetID::UI_Login_Button_New] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "New account button", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Button_Quit] = createAssetInfo(
            {"UI:Login.img/BtQuit/normal/0"}, "Texture", 
            "Quit button", {"IO/UITypes/UILogin.cpp"}, "v92", {}, true);

        asset_map_[AssetID::UI_Login_Button_Homepage] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Homepage button", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Button_PasswdLost] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Lost password button", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Button_EmailLost] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Lost email button", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Button_EmailSave] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Save email button", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Tab_Disabled] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Disabled tab state", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Tab_Enabled] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Enabled tab state", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Field_MapleID] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "MapleID input field", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Field_NexonID] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture", 
            "NexonID input field", {"IO/UITypes/UILogin.cpp"}, "v92", {});

        asset_map_[AssetID::UI_Login_Field_Password] = createAssetInfo(
            {"UI:Login.img/Common/frame"}, "Texture",
            "Password input field", {"IO/UITypes/UILogin.cpp"}, "v92", {});

    }

    // Convenience functions
#ifdef USE_NX
    nl::node GetAsset(AssetID id)
    {
        return AssetRegistry::get().resolve(id);
    }
#endif

    std::string GetAssetPath(AssetID id)
    {
        return AssetRegistry::get().resolveFullPath(id);
    }
}