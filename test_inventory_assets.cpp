#include <iostream>
#include <vector>
#include <string>
#include "Util/NxFiles.h"
#include "includes/NoLifeNx/nlnx/nx.hpp"
#include "includes/NoLifeNx/nlnx/node.hpp"

int main() {
    // Initialize NX files
    NxFiles::init();
    
    std::cout << "=== Inventory UI Asset Check ===\n\n";
    
    // Check container
    nl::node container = nl::nx::ui["UIWindow2.img"]["Item"];
    std::cout << "[" << (!container.name().empty() ? "FOUND" : "MISSING") << "] Container: UI/UIWindow2.img/Item\n";
    
    // Check backgrounds
    std::cout << "\n--- Background Textures ---\n";
    std::vector<std::string> bg_variants = {"productionBackgrnd", "backgrnd", "Backgrnd"};
    for (const auto& bg : bg_variants) {
        nl::node node = container[bg];
        std::cout << "[" << (!node.name().empty() ? "FOUND" : "MISSING") << "] " << bg << "\n";
        
        // Check numbered variants
        for (int i = 2; i <= 3; i++) {
            nl::node numbered = container[bg + std::to_string(i)];
            std::cout << "[" << (!numbered.name().empty() ? "FOUND" : "MISSING") << "] " << bg << i << "\n";
        }
    }
    
    // Check full backgrounds
    for (int i = 1; i <= 3; i++) {
        std::string name = i == 1 ? "FullBackgrnd" : "FullBackgrnd" + std::to_string(i);
        nl::node node = container[name];
        std::cout << "[" << (!node.name().empty() ? "FOUND" : "MISSING") << "] " << name << "\n";
    }
    
    // Check position data
    std::cout << "\n--- Position Data ---\n";
    nl::node pos = container["pos"];
    std::vector<std::string> pos_items = {"slot_col", "slot_pos", "slot_row", "slot_space_x", "slot_space_y"};
    for (const auto& item : pos_items) {
        nl::node node = pos[item];
        std::cout << "[" << (!node.name().empty() ? "FOUND" : "MISSING") << "] pos/" << item << "\n";
    }
    
    // Check new item indicators
    std::cout << "\n--- New Item Indicators ---\n";
    nl::node new_node = container["New"];
    std::cout << "[" << (!new_node["inventory"].name().empty() ? "FOUND" : "MISSING") << "] New/inventory\n";
    std::cout << "[" << (!new_node["Tab0"].name().empty() ? "FOUND" : "MISSING") << "] New/Tab0\n";
    std::cout << "[" << (!new_node["Tab1"].name().empty() ? "FOUND" : "MISSING") << "] New/Tab1\n";
    
    // Check icons
    std::cout << "\n--- Icons ---\n";
    std::cout << "[" << (!container["activeIcon"].name().empty() ? "FOUND" : "MISSING") << "] activeIcon\n";
    std::cout << "[" << (!container["disabled"].name().empty() ? "FOUND" : "MISSING") << "] disabled\n";
    
    // Check tab buttons
    std::cout << "\n--- Tab Buttons ---\n";
    nl::node tab = container["Tab"];
    for (int i = 0; i <= 5; i++) {
        nl::node enabled = tab["enabled"][std::to_string(i)];
        nl::node disabled = tab["disabled"][std::to_string(i)];
        std::cout << "[" << (!enabled.name().empty() ? "FOUND" : "MISSING") << "] Tab/enabled/" << i << "\n";
        std::cout << "[" << (!disabled.name().empty() ? "FOUND" : "MISSING") << "] Tab/disabled/" << i << "\n";
    }
    
    // Check normal mode buttons
    std::cout << "\n--- Normal Mode Buttons ---\n";
    nl::node autobuild = container["AutoBuild"];
    std::vector<std::string> normal_buttons = {"Coin", "Point", "Gather", "Sort", "Full", "Upgrade", "Appraise", "Extract", "Disassemble"};
    for (const auto& btn : normal_buttons) {
        nl::node node = autobuild["button:" + btn];
        std::cout << "[" << (!node.name().empty() ? "FOUND" : "MISSING") << "] AutoBuild/button:" << btn << "\n";
    }
    std::cout << "[" << (!autobuild["anibutton:Toad"].name().empty() ? "FOUND" : "MISSING") << "] AutoBuild/anibutton:Toad\n";
    
    // Check full mode buttons
    std::cout << "\n--- Full Mode Buttons ---\n";
    nl::node fullautobuild = container["FullAutoBuild"];
    std::vector<std::string> full_buttons = {"Small", "Coin", "Point", "Gather", "Sort", "Upgrade", "Appraise", "Extract", "Disassemble", "Cashshop"};
    for (const auto& btn : full_buttons) {
        nl::node node = fullautobuild["button:" + btn];
        std::cout << "[" << (!node.name().empty() ? "FOUND" : "MISSING") << "] FullAutoBuild/button:" << btn << "\n";
    }
    std::cout << "[" << (!fullautobuild["anibutton:Toad"].name().empty() ? "FOUND" : "MISSING") << "] FullAutoBuild/anibutton:Toad\n";
    
    // Check close button
    std::cout << "\n--- Close Button ---\n";
    nl::node close = nl::nx::ui["Basic.img"]["BtClose3"];
    std::cout << "[" << (!close.name().empty() ? "FOUND" : "MISSING") << "] Basic.img/BtClose3\n";
    
    return 0;
}