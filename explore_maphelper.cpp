#include <iostream>
#include <fstream>
#include <string>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

int main() {
    try {
        // Load NX files
        nl::nx::load_all();
        
        std::cout << "=== EXPLORING MAP.NX FOR MAPHELPER.IMG ===" << std::endl;
        
        // Check if Map.nx is loaded
        nl::node map_root = nl::nx::Map;
        if (map_root.size() == 0) {
            std::cout << "ERROR: Map.nx not loaded or empty" << std::endl;
            return 1;
        }
        
        std::cout << "Map.nx loaded successfully with " << map_root.size() << " root items" << std::endl;
        
        // Look for MapHelper.img
        nl::node maphelper = map_root["MapHelper.img"];
        if (maphelper.size() == 0) {
            std::cout << "ERROR: MapHelper.img not found in Map.nx" << std::endl;
            
            // List all available files in Map.nx
            std::cout << "\nAvailable files in Map.nx:" << std::endl;
            int count = 0;
            for (const auto& child : map_root) {
                std::cout << "  - " << child.name() << std::endl;
                count++;
                if (count > 20) {
                    std::cout << "  ... (showing first 20 items)" << std::endl;
                    break;
                }
            }
            return 1;
        }
        
        std::cout << "MapHelper.img found!" << std::endl;
        
        // Check for portal node
        nl::node portal = maphelper["portal"];
        if (portal.size() == 0) {
            std::cout << "ERROR: portal node not found in MapHelper.img" << std::endl;
            
            // List all available nodes in MapHelper.img
            std::cout << "\nAvailable nodes in MapHelper.img:" << std::endl;
            for (const auto& child : maphelper) {
                std::cout << "  - " << child.name() << std::endl;
            }
            return 1;
        }
        
        std::cout << "portal node found!" << std::endl;
        
        // Check for game node
        nl::node game = portal["game"];
        if (game.size() == 0) {
            std::cout << "ERROR: game node not found in portal" << std::endl;
            
            // List all available nodes in portal
            std::cout << "\nAvailable nodes in portal:" << std::endl;
            for (const auto& child : portal) {
                std::cout << "  - " << child.name() << std::endl;
            }
            return 1;
        }
        
        std::cout << "game node found!" << std::endl;
        
        // Now explore the structure under portal/game
        std::cout << "\n=== PORTAL/GAME STRUCTURE ===" << std::endl;
        
        // Check for pv node (regular portals)
        nl::node pv = game["pv"];
        if (pv.size() > 0) {
            std::cout << "pv node found with " << pv.size() << " children:" << std::endl;
            for (const auto& child : pv) {
                std::cout << "  - pv/" << child.name() << std::endl;
            }
            
            // Check for default node in pv
            nl::node pv_default = pv["default"];
            if (pv_default.size() > 0) {
                std::cout << "  pv/default found with " << pv_default.size() << " children:" << std::endl;
                for (const auto& child : pv_default) {
                    std::cout << "    - pv/default/" << child.name() << std::endl;
                }
            } else {
                std::cout << "  pv/default NOT found" << std::endl;
            }
        } else {
            std::cout << "pv node NOT found" << std::endl;
        }
        
        // Check for ph node (hidden portals)
        nl::node ph = game["ph"];
        if (ph.size() > 0) {
            std::cout << "ph node found with " << ph.size() << " children:" << std::endl;
            for (const auto& child : ph) {
                std::cout << "  - ph/" << child.name() << std::endl;
            }
            
            // Check for default node in ph
            nl::node ph_default = ph["default"];
            if (ph_default.size() > 0) {
                std::cout << "  ph/default found with " << ph_default.size() << " children:" << std::endl;
                for (const auto& child : ph_default) {
                    std::cout << "    - ph/default/" << child.name() << std::endl;
                }
            } else {
                std::cout << "  ph/default NOT found" << std::endl;
            }
        } else {
            std::cout << "ph node NOT found" << std::endl;
        }
        
        // List all nodes in game
        std::cout << "\nAll nodes in portal/game:" << std::endl;
        for (const auto& child : game) {
            std::cout << "  - " << child.name() << std::endl;
        }
        
        std::cout << "\n=== EXPLORATION COMPLETE ===" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cout << "Exception: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}