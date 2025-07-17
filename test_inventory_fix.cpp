#include <iostream>
#include "Util/V83UIAssets.h"
#include "includes/NoLifeNx/nlnx/nx.hpp"

int main() {
    // Initialize NX files
    try {
        nl::nx::load_all();
        std::cout << "NX files loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Failed to load NX files: " << e.what() << std::endl;
        return 1;
    }
    
    // Test version detection
    bool is_v83 = ms::V83UIAssets::isV83Mode();
    bool is_v92 = ms::V83UIAssets::isV92Mode();
    
    std::cout << "Version detection:" << std::endl;
    std::cout << "  isV83Mode(): " << (is_v83 ? "true" : "false") << std::endl;
    std::cout << "  isV92Mode(): " << (is_v92 ? "true" : "false") << std::endl;
    
    // Test inventory tab structure
    nl::node Item;
    if (is_v83) {
        Item = nl::nx::UI["UIWindow.img"]["Item"];
        std::cout << "Using V83/V87 UIWindow.img structure" << std::endl;
    } else if (is_v92) {
        Item = nl::nx::UI["UIWindow.img"]["Item"];
        std::cout << "Using V92 UIWindow.img structure" << std::endl;
    } else {
        Item = nl::nx::UI["UIWindow2.img"]["Item"];
        std::cout << "Using modern UIWindow2.img structure" << std::endl;
    }
    
    if (!Item) {
        std::cout << "ERROR: Item node not found!" << std::endl;
        return 1;
    }
    
    std::cout << "Item node found successfully" << std::endl;
    
    // Test tab structure
    nl::node Tab = Item["Tab"];
    if (!Tab) {
        std::cout << "ERROR: Tab node not found!" << std::endl;
        return 1;
    }
    
    std::cout << "Tab node found successfully" << std::endl;
    
    nl::node enabled = Tab["enabled"];
    nl::node disabled = Tab["disabled"];
    
    if (!enabled || !disabled) {
        std::cout << "ERROR: enabled/disabled nodes not found!" << std::endl;
        return 1;
    }
    
    std::cout << "enabled/disabled nodes found successfully" << std::endl;
    
    // Check available tabs
    std::cout << "Available tabs:" << std::endl;
    for (int i = 0; i < 10; i++) {
        std::string tab_num = std::to_string(i);
        nl::node en_tab = enabled[tab_num];
        nl::node dis_tab = disabled[tab_num];
        
        if (en_tab && dis_tab) {
            std::cout << "  Tab " << i << ": enabled=yes, disabled=yes" << std::endl;
        } else if (en_tab) {
            std::cout << "  Tab " << i << ": enabled=yes, disabled=no" << std::endl;
        } else if (dis_tab) {
            std::cout << "  Tab " << i << ": enabled=no, disabled=yes" << std::endl;
        }
    }
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}