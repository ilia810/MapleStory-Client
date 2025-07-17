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

#include "Assets.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#ifdef USE_NX
#include <nlnx/node.hpp>
#endif

// Hash specialization for AssetID enum to work with unordered_map
namespace std {
    template<>
    struct hash<ms::AssetID> {
        size_t operator()(const ms::AssetID& assetId) const {
            return hash<int>()(static_cast<int>(assetId));
        }
    };
}

namespace ms
{
    // Centralized asset management system that replaces hardcoded string paths
    // with type-safe logical identifiers and provides version compatibility
    class AssetRegistry
    {
    public:
        // Singleton access
        static AssetRegistry& get();
        
        // Initialize the registry by loading the master asset map
        bool load(const std::string& map_filepath = "assets_master.yml");
        
        // Check if the registry has been initialized
        bool is_loaded() const;
        
        // Retrieve the resolved path for a logical asset ID
        // Returns the first valid path from the prioritized list
        const std::string& getPath(AssetID id) const;
        
        // Get all possible paths for an asset (for debugging/analysis)
        const std::vector<std::string>& getAllPaths(AssetID id) const;
        
        // Check if an asset has any valid paths available
        bool hasValidPath(AssetID id) const;
        
        // Get asset metadata
        const std::string& getAssetType(AssetID id) const;
        const std::string& getAssetNotes(AssetID id) const;
        
        // Utility functions for path resolution
        std::string resolveFullPath(AssetID id) const;
        
#ifdef USE_NX
        // Resolve an asset ID to an actual nl::node
        nl::node resolve(AssetID id) const;
        
        // Check if a specific path is valid in the loaded nx data
        bool isPathValid(const std::string& path) const;
#endif
        
        // Error handling
        enum class ErrorCode
        {
            NONE,
            FILE_NOT_FOUND,
            PARSE_ERROR,
            ASSET_NOT_FOUND,
            NO_VALID_PATHS,
            NX_NOT_LOADED
        };
        
        ErrorCode getLastError() const;
        const std::string& getLastErrorMessage() const;
        
    private:
        AssetRegistry() = default;
        ~AssetRegistry() = default;
        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;
        
        // Internal data structure to hold asset information
        struct AssetInfo
        {
            std::vector<std::string> prioritized_paths;  // Multiple paths for fallback
            std::string asset_type;                      // Texture, Node, Audio, etc.
            std::string notes;                           // Description and compatibility info
            std::vector<std::string> consumers;          // Files that use this asset
            std::string version_compatibility;           // v83, post-v83, etc.
            std::vector<std::string> fallback_paths;     // Alternative asset locations
            bool allow_hybrid_resolve = false;           // Enable automatic hybrid container traversal
        };
        
        // Asset ID to info mapping
        std::unordered_map<AssetID, AssetInfo> asset_map_;
        
        // String to AssetID mapping for YAML parsing
        std::unordered_map<std::string, AssetID> name_to_id_;
        
        // Error handling
        mutable ErrorCode last_error_ = ErrorCode::NONE;
        mutable std::string last_error_message_;
        
        // Loading state
        bool loaded_ = false;
        
        // Helper methods
        bool parseYAMLFile(const std::string& filepath);
        void initializeNameMapping();
        void initializeHardcodedMappings();
        void setError(ErrorCode code, const std::string& message) const;
        
        // Path processing helpers
        std::string constructFullPath(const std::string& root_node, const std::string& path) const;
        std::pair<std::string, std::string> parseAssetPath(const std::string& full_path) const;
        
#ifdef USE_NX
        // NX-specific path resolution
        nl::node resolvePathToNode(const std::string& full_path) const;
#endif
        
        // Default empty strings for error cases
        static const std::string empty_string_;
        static const std::vector<std::string> empty_vector_;
    };
    
    // Convenience function for direct asset resolution
#ifdef USE_NX
    nl::node GetAsset(AssetID id);
#endif
    
    // Asset path helper for legacy code migration
    std::string GetAssetPath(AssetID id);
}