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
#include <iostream>
#include <cassert>

namespace ms
{
    // Simple test framework
    class AssetRegistryTest
    {
    public:
        static void runAllTests()
        {
            std::cout << "[AssetRegistryTest] Starting unit tests..." << std::endl;
            
            testSingletonAccess();
            testLoadRegistry();
            testAssetResolution();
            testErrorHandling();
            testFallbackLogic();
            
            std::cout << "[AssetRegistryTest] All tests passed!" << std::endl;
        }

    private:
        static void testSingletonAccess()
        {
            std::cout << "[Test] Singleton access..." << std::endl;
            
            AssetRegistry& registry1 = AssetRegistry::get();
            AssetRegistry& registry2 = AssetRegistry::get();
            
            // Should be the same instance
            assert(&registry1 == &registry2);
            
            std::cout << "[Test] Singleton access - PASSED" << std::endl;
        }

        static void testLoadRegistry()
        {
            std::cout << "[Test] Registry loading..." << std::endl;
            
            AssetRegistry& registry = AssetRegistry::get();
            
            // Should not be loaded initially
            assert(!registry.is_loaded());
            
            // Load the registry
            bool loaded = registry.load();
            assert(loaded);
            assert(registry.is_loaded());
            
            std::cout << "[Test] Registry loading - PASSED" << std::endl;
        }

        static void testAssetResolution()
        {
            std::cout << "[Test] Asset resolution..." << std::endl;
            
            AssetRegistry& registry = AssetRegistry::get();
            
            // Test known assets
            const std::string& login_bg_path = registry.getPath(AssetID::UI_Login_Background);
            assert(!login_bg_path.empty());
            
            const std::string& shop_bg_path = registry.getPath(AssetID::UI_Shop_Background);
            assert(!shop_bg_path.empty());
            
            // Test full path resolution
            std::string full_path = registry.resolveFullPath(AssetID::UI_Login_Background);
            assert(!full_path.empty());
            
            // Test metadata access
            const std::string& asset_type = registry.getAssetType(AssetID::UI_Login_Background);
            assert(asset_type == "Texture");
            
            std::cout << "[Test] Asset resolution - PASSED" << std::endl;
        }

        static void testErrorHandling()
        {
            std::cout << "[Test] Error handling..." << std::endl;
            
            AssetRegistry& registry = AssetRegistry::get();
            
            // Test with invalid asset ID (cast to valid range to avoid compiler issues)
            AssetID invalid_id = static_cast<AssetID>(999999);
            const std::string& invalid_path = registry.getPath(invalid_id);
            assert(invalid_path.empty());
            
            // Check error state
            assert(registry.getLastError() == AssetRegistry::ErrorCode::ASSET_NOT_FOUND);
            assert(!registry.getLastErrorMessage().empty());
            
            std::cout << "[Test] Error handling - PASSED" << std::endl;
        }

        static void testFallbackLogic()
        {
            std::cout << "[Test] Fallback logic..." << std::endl;
            
            AssetRegistry& registry = AssetRegistry::get();
            
            // Test assets with multiple paths
            const auto& all_paths = registry.getAllPaths(AssetID::UI_Login_Background);
            assert(all_paths.size() > 1);  // Should have fallback paths
            
            // Test path validation
            bool has_valid = registry.hasValidPath(AssetID::UI_Login_Background);
            // Note: This test depends on NX files being loaded, so we just check it doesn't crash
            (void)has_valid;  // Suppress unused variable warning
            
            std::cout << "[Test] Fallback logic - PASSED" << std::endl;
        }
    };

    // Test runner function that can be called from main or debug code
    void runAssetRegistryTests()
    {
        AssetRegistryTest::runAllTests();
    }
}