#include <iostream>
#include <vector>
#include <string>
#include "../Util/AssetRegistry.h"

struct AssetCheck {
    std::string path;
    std::string description;
};

int main() {
    std::vector<AssetCheck> assets = {
        // Container Node
        {"UI/UIWindow2.img/Item", "Container Node"},
        
        // Background Textures
        {"UI/UIWindow2.img/Item/productionBackgrnd", "Background - productionBackgrnd"},
        {"UI/UIWindow2.img/Item/backgrnd", "Background - backgrnd"},
        {"UI/UIWindow2.img/Item/Backgrnd", "Background - Backgrnd"},
        {"UI/UIWindow2.img/Item/productionBackgrnd2", "Background - productionBackgrnd2"},
        {"UI/UIWindow2.img/Item/backgrnd2", "Background - backgrnd2"},
        {"UI/UIWindow2.img/Item/Backgrnd2", "Background - Backgrnd2"},
        {"UI/UIWindow2.img/Item/productionBackgrnd3", "Background - productionBackgrnd3"},
        {"UI/UIWindow2.img/Item/backgrnd3", "Background - backgrnd3"},
        {"UI/UIWindow2.img/Item/Backgrnd3", "Background - Backgrnd3"},
        {"UI/UIWindow2.img/Item/FullBackgrnd", "Full Background 1"},
        {"UI/UIWindow2.img/Item/FullBackgrnd2", "Full Background 2"},
        {"UI/UIWindow2.img/Item/FullBackgrnd3", "Full Background 3"},
        
        // Position Data
        {"UI/UIWindow2.img/Item/pos/slot_col", "Position - slot_col"},
        {"UI/UIWindow2.img/Item/pos/slot_pos", "Position - slot_pos"},
        {"UI/UIWindow2.img/Item/pos/slot_row", "Position - slot_row"},
        {"UI/UIWindow2.img/Item/pos/slot_space_x", "Position - slot_space_x"},
        {"UI/UIWindow2.img/Item/pos/slot_space_y", "Position - slot_space_y"},
        
        // New Item Indicators
        {"UI/UIWindow2.img/Item/New/inventory", "New Item - inventory"},
        {"UI/UIWindow2.img/Item/New/Tab0", "New Item - Tab0"},
        {"UI/UIWindow2.img/Item/New/Tab1", "New Item - Tab1"},
        
        // Other Icons
        {"UI/UIWindow2.img/Item/activeIcon", "Active Icon (projectile)"},
        {"UI/UIWindow2.img/Item/disabled", "Disabled Slot Icon"},
        
        // Tab Buttons - Enabled
        {"UI/UIWindow2.img/Item/Tab/enabled/0", "Tab Button - Enabled 0"},
        {"UI/UIWindow2.img/Item/Tab/enabled/1", "Tab Button - Enabled 1"},
        {"UI/UIWindow2.img/Item/Tab/enabled/2", "Tab Button - Enabled 2"},
        {"UI/UIWindow2.img/Item/Tab/enabled/3", "Tab Button - Enabled 3"},
        {"UI/UIWindow2.img/Item/Tab/enabled/4", "Tab Button - Enabled 4"},
        {"UI/UIWindow2.img/Item/Tab/enabled/5", "Tab Button - Enabled 5"},
        
        // Tab Buttons - Disabled
        {"UI/UIWindow2.img/Item/Tab/disabled/0", "Tab Button - Disabled 0"},
        {"UI/UIWindow2.img/Item/Tab/disabled/1", "Tab Button - Disabled 1"},
        {"UI/UIWindow2.img/Item/Tab/disabled/2", "Tab Button - Disabled 2"},
        {"UI/UIWindow2.img/Item/Tab/disabled/3", "Tab Button - Disabled 3"},
        {"UI/UIWindow2.img/Item/Tab/disabled/4", "Tab Button - Disabled 4"},
        {"UI/UIWindow2.img/Item/Tab/disabled/5", "Tab Button - Disabled 5"},
        
        // Normal Mode Buttons
        {"UI/UIWindow2.img/Item/AutoBuild/button:Coin", "Normal Button - Coin"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Point", "Normal Button - Point"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Gather", "Normal Button - Gather"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Sort", "Normal Button - Sort"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Full", "Normal Button - Full"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Upgrade", "Normal Button - Upgrade"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Appraise", "Normal Button - Appraise"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Extract", "Normal Button - Extract"},
        {"UI/UIWindow2.img/Item/AutoBuild/button:Disassemble", "Normal Button - Disassemble"},
        {"UI/UIWindow2.img/Item/AutoBuild/anibutton:Toad", "Normal Button - Toad"},
        
        // Full Mode Buttons
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Small", "Full Button - Small"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Coin", "Full Button - Coin"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Point", "Full Button - Point"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Gather", "Full Button - Gather"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Sort", "Full Button - Sort"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Upgrade", "Full Button - Upgrade"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Appraise", "Full Button - Appraise"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Extract", "Full Button - Extract"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Disassemble", "Full Button - Disassemble"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/anibutton:Toad", "Full Button - Toad"},
        {"UI/UIWindow2.img/Item/FullAutoBuild/button:Cashshop", "Full Button - Cashshop"},
        
        // Close Button
        {"UI/Basic.img/BtClose3", "Close Button"}
    };
    
    AssetRegistry& registry = AssetRegistry::get();
    
    std::cout << "=== Inventory UI Asset Check ===\n\n";
    
    int found = 0;
    int missing = 0;
    
    for (const auto& asset : assets) {
        bool exists = registry.isPathValid(asset.path);
        
        if (exists) {
            std::cout << "[FOUND] " << asset.description << "\n";
            std::cout << "        Path: " << asset.path << "\n";
            found++;
        } else {
            std::cout << "[MISSING] " << asset.description << "\n";
            std::cout << "          Path: " << asset.path << "\n";
            missing++;
        }
    }
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "Total assets checked: " << assets.size() << "\n";
    std::cout << "Found: " << found << "\n";
    std::cout << "Missing: " << missing << "\n";
    
    return 0;
}