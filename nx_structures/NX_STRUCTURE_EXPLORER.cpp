// NX Structure Explorer Tool
// Use this to dump the actual structure of v83 NX files

#include <iostream>
#include <string>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void printNodeTree(nl::node node, const std::string& path = "", int depth = 0) {
    std::string indent(depth * 2, ' ');
    
    // Print current node info
    std::cout << indent << path;
    
    // Show node type
    switch (node.data_type()) {
        case nl::node::type::none:
            std::cout << " [CONTAINER]";
            break;
        case nl::node::type::integer:
            std::cout << " [INT: " << node.get_integer() << "]";
            break;
        case nl::node::type::real:
            std::cout << " [REAL: " << node.get_real() << "]";
            break;
        case nl::node::type::string:
            std::cout << " [STRING: " << node.get_string() << "]";
            break;
        case nl::node::type::vector:
            std::cout << " [VECTOR: " << node.get_vector().x << "," << node.get_vector().y << "]";
            break;
        case nl::node::type::bitmap:
            std::cout << " [BITMAP: " << node.get_bitmap().width() << "x" << node.get_bitmap().height() << "]";
            break;
        case nl::node::type::audio:
            std::cout << " [AUDIO]";
            break;
    }
    
    if (node.size() > 0) {
        std::cout << " (" << node.size() << " children)";
    }
    
    std::cout << std::endl;
    
    // Recursively print children (limit depth to avoid too much output)
    if (depth < 5 && node.size() > 0) {
        for (const auto& child : node) {
            printNodeTree(child, child.name(), depth + 1);
        }
    }
}

void exploreLoginAssets() {
    std::cout << "=== EXPLORING LOGIN.IMG ===" << std::endl;
    
    nl::node login = nl::nx::UI["Login.img"];
    if (!login) {
        std::cout << "ERROR: Login.img not found!" << std::endl;
        return;
    }
    
    std::cout << "Login.img structure:" << std::endl;
    printNodeTree(login, "Login.img");
    
    std::cout << "\n=== EXPLORING MAP LOGIN ASSETS ===" << std::endl;
    
    nl::node mapLogin = nl::nx::Map["Obj"]["login.img"];
    if (mapLogin) {
        std::cout << "Map/Obj/login.img structure:" << std::endl;
        printNodeTree(mapLogin, "Map/Obj/login.img");
    }
    
    nl::node mapBack = nl::nx::Map["Back"]["login"];
    if (mapBack) {
        std::cout << "Map/Back/login structure:" << std::endl;
        printNodeTree(mapBack, "Map/Back/login");
    }
    
    std::cout << "\n=== EXPLORING UIWINDOW ===" << std::endl;
    
    nl::node uiwindow = nl::nx::UI["UIWindow.img"];
    if (uiwindow) {
        std::cout << "UIWindow.img exists (v83 mode)" << std::endl;
        // Just show top level to avoid too much output
        for (const auto& child : uiwindow) {
            std::cout << "  - " << child.name() << " [" << (int)child.data_type() << "]";
            if (child.size() > 0) {
                std::cout << " (" << child.size() << " children)";
            }
            std::cout << std::endl;
        }
    }
    
    nl::node uiwindow2 = nl::nx::UI["UIWindow2.img"];
    if (uiwindow2) {
        std::cout << "UIWindow2.img exists (modern mode)" << std::endl;
    }
}

// Add this function to UILogin.cpp temporarily for debugging:
void debugLoginAssets() {
    exploreLoginAssets();
}