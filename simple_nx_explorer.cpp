// Simple NX Explorer - minimal version for quick compilation
#include <iostream>
#include <string>
#include <iomanip>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void printNode(const nl::node& node, const std::string& path = "", int depth = 0) {
    if (!node || depth > 3) return;  // Limit depth to avoid too much output
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << path << " [";
    
    switch (node.data_type()) {
        case nl::node::type::none:
            std::cout << "container, " << node.size() << " children";
            break;
        case nl::node::type::bitmap:
            {
                auto bmp = node.get_bitmap();
                std::cout << "bitmap " << bmp.width() << "x" << bmp.height();
            }
            break;
        case nl::node::type::integer:
            std::cout << "int: " << node.get_integer();
            break;
        case nl::node::type::string:
            std::cout << "string: " << node.get_string();
            break;
        case nl::node::type::vector:
            {
                auto v = node.get_vector();
                std::cout << "vector: " << v.x << "," << v.y;
            }
            break;
        default:
            std::cout << "other";
    }
    
    std::cout << "]" << std::endl;
    
    // Print children
    for (const auto& child : node) {
        printNode(child, path + "/" + child.name(), depth + 1);
    }
}

int main() {
    try {
        // Try to load NX files from common locations
        std::cout << "Loading NX files..." << std::endl;
        
        // Try different possible locations
        bool loaded = false;
        std::vector<std::string> paths = {"../nx_files", "./nx_files", "../Data", "./Data"};
        
        for (const auto& path : paths) {
            try {
                nl::nx::load_all(path);
                std::cout << "Successfully loaded from: " << path << std::endl;
                loaded = true;
                break;
            } catch (...) {
                // Try next path
            }
        }
        
        if (!loaded) {
            std::cout << "Failed to load NX files. Make sure they're in one of these directories:" << std::endl;
            for (const auto& path : paths) {
                std::cout << "  " << path << std::endl;
            }
            return 1;
        }
        
        // Explore key login assets
        std::cout << "\n=== LOGIN.IMG STRUCTURE ===" << std::endl;
        nl::node login = nl::nx::UI["Login.img"];
        if (login) {
            printNode(login, "UI/Login.img");
        } else {
            std::cout << "Login.img not found!" << std::endl;
        }
        
        std::cout << "\n=== MAP LOGIN BACKGROUND ===" << std::endl;
        nl::node mapLogin = nl::nx::Map["Obj"]["login.img"];
        if (mapLogin) {
            printNode(mapLogin, "Map/Obj/login.img");
        } else {
            std::cout << "Map/Obj/login.img not found!" << std::endl;
        }
        
        std::cout << "\n=== CHECKING V83 INDICATORS ===" << std::endl;
        nl::node uiWindow = nl::nx::UI["UIWindow.img"];
        nl::node uiWindow2 = nl::nx::UI["UIWindow2.img"];
        
        std::cout << "UIWindow.img exists: " << (uiWindow ? "YES (v83 mode)" : "NO") << std::endl;
        std::cout << "UIWindow2.img exists: " << (uiWindow2 ? "YES (modern mode)" : "NO") << std::endl;
        
        if (uiWindow) {
            std::cout << "\n=== UIWINDOW.IMG STRUCTURE (v83) ===" << std::endl;
            printNode(uiWindow, "UI/UIWindow.img");
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nPress any key to continue..." << std::endl;
    std::cin.get();
    return 0;
}