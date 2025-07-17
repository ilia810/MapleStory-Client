#include <iostream>
#include <string>
#include <vector>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void printNodeInfo(const nl::node& node, const std::string& path, int maxDepth = 2, int currentDepth = 0) {
    if (currentDepth > maxDepth) return;
    
    std::string indent(currentDepth * 2, ' ');
    std::cout << indent << path;
    
    if (node.data_type() == nl::node::type::bitmap) {
        std::cout << " [BITMAP]";
    } else if (node.data_type() == nl::node::type::string) {
        std::cout << " [STRING: " << node.get_string() << "]";
    } else if (node.data_type() == nl::node::type::integer) {
        std::cout << " [INT: " << node.get_integer() << "]";
    } else if (node.size() > 0) {
        std::cout << " [CONTAINER: " << node.size() << " children]";
    }
    std::cout << std::endl;
    
    if (currentDepth < maxDepth && node.size() > 0) {
        for (const auto& child : node) {
            printNodeInfo(child, path + "/" + child.name(), maxDepth, currentDepth + 1);
        }
    }
}

void checkUIAssets() {
    std::cout << "=== V83 UI Asset Check ===" << std::endl;
    
    // Check UI.nx structure
    std::cout << "\n--- UI.nx Structure ---" << std::endl;
    if (nl::nx::UI) {
        std::cout << "UI.nx loaded successfully" << std::endl;
        
        // Check Login.img
        std::cout << "\n--- Login.img ---" << std::endl;
        nl::node login = nl::nx::UI["Login.img"];
        if (login) {
            printNodeInfo(login, "Login.img", 2);
        } else {
            std::cout << "Login.img NOT FOUND!" << std::endl;
        }
        
        // Check UIWindow.img (v83)
        std::cout << "\n--- UIWindow.img (v83) ---" << std::endl;
        nl::node uiwindow = nl::nx::UI["UIWindow.img"];
        if (uiwindow) {
            printNodeInfo(uiwindow, "UIWindow.img", 2);
        } else {
            std::cout << "UIWindow.img NOT FOUND!" << std::endl;
        }
        
        // Check UIWindow2.img (modern)
        std::cout << "\n--- UIWindow2.img (modern) ---" << std::endl;
        nl::node uiwindow2 = nl::nx::UI["UIWindow2.img"];
        if (uiwindow2) {
            printNodeInfo(uiwindow2, "UIWindow2.img", 2);
        } else {
            std::cout << "UIWindow2.img NOT FOUND (expected for v83)" << std::endl;
        }
        
        // Check Basic.img
        std::cout << "\n--- Basic.img ---" << std::endl;
        nl::node basic = nl::nx::UI["Basic.img"];
        if (basic) {
            printNodeInfo(basic, "Basic.img", 2);
        } else {
            std::cout << "Basic.img NOT FOUND!" << std::endl;
        }
        
        // Check StatusBar.img
        std::cout << "\n--- StatusBar.img ---" << std::endl;
        nl::node statusbar = nl::nx::UI["StatusBar.img"];
        if (statusbar) {
            printNodeInfo(statusbar, "StatusBar.img", 2);
        } else {
            std::cout << "StatusBar.img NOT FOUND!" << std::endl;
        }
    } else {
        std::cout << "ERROR: UI.nx not loaded!" << std::endl;
    }
    
    // Check Map.nx for backgrounds
    std::cout << "\n\n--- Map.nx Background Check ---" << std::endl;
    if (nl::nx::Map) {
        std::cout << "Map.nx loaded successfully" << std::endl;
        
        // Check login backgrounds
        nl::node loginBg = nl::nx::Map["Obj"]["login.img"];
        if (loginBg) {
            std::cout << "\nMap/Obj/login.img found:" << std::endl;
            printNodeInfo(loginBg, "login.img", 2);
        }
        
        nl::node backLogin = nl::nx::Map["Back"]["login"];
        if (backLogin) {
            std::cout << "\nMap/Back/login found:" << std::endl;
            printNodeInfo(backLogin, "login", 2);
        }
        
        nl::node uiLogin = nl::nx::Map["Back"]["UI_login.img"];
        if (uiLogin) {
            std::cout << "\nMap/Back/UI_login.img found:" << std::endl;
            printNodeInfo(uiLogin, "UI_login.img", 2);
        }
    }
}

int main() {
    try {
        // Initialize NX files
        nl::nx::load_all();
        
        checkUIAssets();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}