#include "CheckInventoryAssets.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "Util/NxFiles.h"
#include "includes/NoLifeNx/nlnx/nx.hpp"
#include "includes/NoLifeNx/nlnx/node.hpp"

struct AssetCheck {
    std::string path;
    std::string description;
    bool exists;
};

static bool checkNode(const std::string& path) {
    try {
        // Parse the path
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
        
        if (parts.empty()) return false;
        
        // Navigate the node tree
        nl::node node;
        if (parts[0] == "UI" || parts[0] == "UI.nx") {
            node = nl::nx::ui;
        } else {
            return false;
        }
        
        // Navigate through the path
        for (size_t i = 1; i < parts.size(); i++) {
            node = node[parts[i]];
            if (node.name().empty()) {
                return false;
            }
        }
        
        return !node.name().empty();
    } catch (...) {
        return false;
    }
}

void CheckInventoryAssets::runCheck() {
    std::cout << "=== MapleStory Inventory UI Asset Check ===\n\n";
    std::cout << "Initializing NX files...\n";
    
    try {
        NxFiles::init();
        std::cout << "NX files loaded successfully!\n\n";
    } catch (...) {
        std::cout << "ERROR: Failed to initialize NX files!\n";
        return 1;
    }
    
    std::vector<std::pair<std::string, std::vector<AssetCheck>>> categories = {
        {"Main Container", {
            {"UI/UIWindow2.img/Item", "Main container node"}
        }},
        
        {"Background Textures", {
            {"UI/UIWindow2.img/Item/productionBackgrnd", "Background - productionBackgrnd"},
            {"UI/UIWindow2.img/Item/backgrnd", "Background - backgrnd (fallback)"},
            {"UI/UIWindow2.img/Item/Backgrnd", "Background - Backgrnd (fallback)"},
            {"UI/UIWindow2.img/Item/productionBackgrnd2", "Background 2 - production"},
            {"UI/UIWindow2.img/Item/backgrnd2", "Background 2 - fallback"},
            {"UI/UIWindow2.img/Item/Backgrnd2", "Background 2 - fallback caps"},
            {"UI/UIWindow2.img/Item/productionBackgrnd3", "Background 3 - production"},
            {"UI/UIWindow2.img/Item/backgrnd3", "Background 3 - fallback"},
            {"UI/UIWindow2.img/Item/Backgrnd3", "Background 3 - fallback caps"},
            {"UI/UIWindow2.img/Item/FullBackgrnd", "Full mode background 1"},
            {"UI/UIWindow2.img/Item/FullBackgrnd2", "Full mode background 2"},
            {"UI/UIWindow2.img/Item/FullBackgrnd3", "Full mode background 3"}
        }},
        
        {"Position Data", {
            {"UI/UIWindow2.img/Item/pos/slot_col", "Slot column count"},
            {"UI/UIWindow2.img/Item/pos/slot_pos", "Slot position"},
            {"UI/UIWindow2.img/Item/pos/slot_row", "Slot row count"},
            {"UI/UIWindow2.img/Item/pos/slot_space_x", "Slot X spacing"},
            {"UI/UIWindow2.img/Item/pos/slot_space_y", "Slot Y spacing"}
        }},
        
        {"New Item Indicators", {
            {"UI/UIWindow2.img/Item/New/inventory", "New item inventory indicator"},
            {"UI/UIWindow2.img/Item/New/Tab0", "New item tab disabled"},
            {"UI/UIWindow2.img/Item/New/Tab1", "New item tab enabled"}
        }},
        
        {"Icons", {
            {"UI/UIWindow2.img/Item/activeIcon", "Active/projectile icon"},
            {"UI/UIWindow2.img/Item/disabled", "Disabled slot overlay"}
        }},
        
        {"Tab Buttons", {
            {"UI/UIWindow2.img/Item/Tab/enabled/0", "Tab 0 enabled (Equip)"},
            {"UI/UIWindow2.img/Item/Tab/enabled/1", "Tab 1 enabled (Use)"},
            {"UI/UIWindow2.img/Item/Tab/enabled/2", "Tab 2 enabled (Etc)"},
            {"UI/UIWindow2.img/Item/Tab/enabled/3", "Tab 3 enabled (Setup)"},
            {"UI/UIWindow2.img/Item/Tab/enabled/4", "Tab 4 enabled (Cash)"},
            {"UI/UIWindow2.img/Item/Tab/enabled/5", "Tab 5 enabled (Dec)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/0", "Tab 0 disabled (Equip)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/1", "Tab 1 disabled (Use)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/2", "Tab 2 disabled (Etc)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/3", "Tab 3 disabled (Setup)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/4", "Tab 4 disabled (Cash)"},
            {"UI/UIWindow2.img/Item/Tab/disabled/5", "Tab 5 disabled (Dec)"}
        }},
        
        {"Normal Mode Buttons", {
            {"UI/UIWindow2.img/Item/AutoBuild/button:Coin", "Coin button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Point", "Point button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Gather", "Gather button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Sort", "Sort button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Full", "Full mode button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Upgrade", "Upgrade button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Appraise", "Appraise button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Extract", "Extract button"},
            {"UI/UIWindow2.img/Item/AutoBuild/button:Disassemble", "Disassemble button"},
            {"UI/UIWindow2.img/Item/AutoBuild/anibutton:Toad", "Toad animated button"}
        }},
        
        {"Full Mode Buttons", {
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Small", "Small mode button"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Coin", "Coin button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Point", "Point button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Gather", "Gather button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Sort", "Sort button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Upgrade", "Upgrade button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Appraise", "Appraise button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Extract", "Extract button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Disassemble", "Disassemble button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/anibutton:Toad", "Toad button (full)"},
            {"UI/UIWindow2.img/Item/FullAutoBuild/button:Cashshop", "Cash shop button"}
        }},
        
        {"UI Controls", {
            {"UI/Basic.img/BtClose3", "Close button"}
        }}
    };
    
    int totalAssets = 0;
    int foundAssets = 0;
    
    // Check each category
    for (auto& category : categories) {
        std::cout << "--- " << category.first << " ---\n";
        int categoryFound = 0;
        
        for (auto& asset : category.second) {
            asset.exists = checkNode(asset.path);
            
            std::cout << "[" << std::setw(7) << (asset.exists ? "FOUND" : "MISSING") << "] "
                      << std::left << std::setw(50) << asset.description
                      << " (" << asset.path << ")\n";
            
            totalAssets++;
            if (asset.exists) {
                foundAssets++;
                categoryFound++;
            }
        }
        
        std::cout << "Subtotal: " << categoryFound << "/" << category.second.size() << " found\n\n";
    }
    
    // Print summary
    std::cout << "=== SUMMARY ===\n";
    std::cout << "Total assets checked: " << totalAssets << "\n";
    std::cout << "Found: " << foundAssets << "\n";
    std::cout << "Missing: " << (totalAssets - foundAssets) << "\n";
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (foundAssets * 100.0 / totalAssets) << "%\n";
    
    // Check for critical missing assets
    if (foundAssets < totalAssets) {
        std::cout << "\nWARNING: Some inventory UI assets are missing!\n";
        std::cout << "This may cause the inventory to not display correctly.\n";
        
        // Check if any background exists
        bool hasBackground = false;
        for (const auto& bg : {"productionBackgrnd", "backgrnd", "Backgrnd"}) {
            if (checkNode(std::string("UI/UIWindow2.img/Item/") + bg)) {
                hasBackground = true;
                std::cout << "\nNote: Using fallback background '" << bg << "'\n";
                break;
            }
        }
        
        if (!hasBackground) {
            std::cout << "\nCRITICAL: No background texture found! Inventory UI may not render.\n";
        }
    } else {
        std::cout << "\nAll inventory UI assets found successfully!\n";
    }
}