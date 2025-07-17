#include <iostream>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#include <nlnx/file.hpp>
#include "Util/NxFiles.h"
#include "Util/Misc.h"
#include "Constants.h"
#include "Configuration.h"

void print_node_info(const std::string& path, nl::node node) {
    std::cout << "[NODE] " << path << ": ";
    if (!node) {
        std::cout << "null (not found)" << std::endl;
        return;
    }
    
    std::cout << "type=" << (int)node.data_type() << " size=" << node.size();
    
    if (node.data_type() == nl::node::type::bitmap) {
        std::cout << " (bitmap)";
    } else if (node.data_type() == nl::node::type::none && node.size() > 0) {
        std::cout << " (container with " << node.size() << " children)";
    } else if (node.data_type() == nl::node::type::none) {
        std::cout << " (empty/null)";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=== V92 Detection Test ===" << std::endl;
    
    // Initialize configuration
    ms::Configuration::get();
    
    // Initialize NX files
    std::cout << "\n1. Initializing NX files..." << std::endl;
    ms::Error error = ms::NxFiles::init();
    if (error.get_result() != ms::Error::Code::NONE) {
        std::cout << "Failed to initialize NX files: " << error.get_message() << std::endl;
        return 1;
    }
    std::cout << "NX files initialized successfully" << std::endl;
    
    // Test basic UI access
    std::cout << "\n2. Testing basic UI access..." << std::endl;
    nl::node ui = nl::nx::UI;
    if (!ui) {
        std::cout << "ERROR: nl::nx::UI is null!" << std::endl;
        return 1;
    }
    print_node_info("UI", ui);
    
    nl::node login = nl::nx::UI["Login.img"];
    print_node_info("UI/Login.img", login);
    
    if (!login) {
        std::cout << "ERROR: Login.img not found!" << std::endl;
        return 1;
    }
    
    // Test Title section (v92 specific)
    std::cout << "\n3. Testing Title section (v92 specific)..." << std::endl;
    nl::node title = login["Title"];
    print_node_info("UI/Login.img/Title", title);
    
    if (title && title.data_type() != nl::node::type::none) {
        std::cout << "Title section is valid, checking buttons..." << std::endl;
        
        // Check specific buttons in Title
        nl::node btLogin = title["BtLogin"];
        print_node_info("UI/Login.img/Title/BtLogin", btLogin);
        
        nl::node btQuit = title["BtQuit"];
        print_node_info("UI/Login.img/Title/BtQuit", btQuit);
        
        nl::node btHomePage = title["BtHomePage"];
        print_node_info("UI/Login.img/Title/BtHomePage", btHomePage);
        
        // Check button structure
        if (btLogin && btLogin.data_type() != nl::node::type::none) {
            nl::node normal = btLogin["normal"];
            print_node_info("UI/Login.img/Title/BtLogin/normal", normal);
            if (normal) {
                nl::node normal0 = normal["0"];
                print_node_info("UI/Login.img/Title/BtLogin/normal/0", normal0);
            }
        }
    }
    
    // Test Common section
    std::cout << "\n4. Testing Common section..." << std::endl;
    nl::node common = login["Common"];
    print_node_info("UI/Login.img/Common", common);
    
    if (common) {
        nl::node frame = common["frame"];
        print_node_info("UI/Login.img/Common/frame", frame);
        
        nl::node btStart = common["BtStart"];
        print_node_info("UI/Login.img/Common/BtStart", btStart);
    }
    
    // Test v92 detection logic
    std::cout << "\n5. Testing v92 detection logic..." << std::endl;
    bool isV92 = false;
    
    // Check if Title section exists and has data
    if (title) {
        std::cout << "Title data_type: " << (int)title.data_type() << std::endl;
        std::cout << "Title size: " << title.size() << std::endl;
        
        // The issue might be that Title exists but is empty or has wrong type
        if (title.data_type() != nl::node::type::none || title.size() > 0) {
            std::cout << "Title section is valid" << std::endl;
            
            // Check each button individually
            nl::node btLogin = title["BtLogin"];
            nl::node btQuit = title["BtQuit"];
            nl::node btHomePage = title["BtHomePage"];
            
            std::cout << "BtLogin type: " << (int)btLogin.data_type() << std::endl;
            std::cout << "BtQuit type: " << (int)btQuit.data_type() << std::endl;
            std::cout << "BtHomePage type: " << (int)btHomePage.data_type() << std::endl;
            
            if ((btLogin && btLogin.data_type() != nl::node::type::none) ||
                (btQuit && btQuit.data_type() != nl::node::type::none) ||
                (btHomePage && btHomePage.data_type() != nl::node::type::none)) {
                isV92 = true;
                std::cout << "V92 specific buttons found in Title section" << std::endl;
            }
        }
    }
    
    std::cout << "\nResult: " << (isV92 ? "V92 DETECTED" : "V92 NOT DETECTED") << std::endl;
    
    // Test the actual V83UIAssets detection functions
    std::cout << "\n6. Testing V83UIAssets detection functions..." << std::endl;
    bool v92Mode = ms::V83UIAssets::isV92Mode();
    bool v83Mode = ms::V83UIAssets::isV83Mode();
    
    std::cout << "V83UIAssets::isV92Mode(): " << (v92Mode ? "true" : "false") << std::endl;
    std::cout << "V83UIAssets::isV83Mode(): " << (v83Mode ? "true" : "false") << std::endl;
    
    // Test WorldSelect assets
    std::cout << "\n7. Testing WorldSelect assets..." << std::endl;
    nl::node worldSelect = login["WorldSelect"];
    print_node_info("UI/Login.img/WorldSelect", worldSelect);
    
    if (worldSelect) {
        nl::node btChannel = worldSelect["BtChannel"];
        print_node_info("UI/Login.img/WorldSelect/BtChannel", btChannel);
        
        nl::node selectedWorld = worldSelect["selectedWorld"];
        print_node_info("UI/Login.img/WorldSelect/selectedWorld", selectedWorld);
        
        nl::node scroll = worldSelect["scroll"];
        print_node_info("UI/Login.img/WorldSelect/scroll", scroll);
        
        // Check scroll subfolders for world textures
        if (scroll) {
            for (int i = 0; i < 23; i++) {
                std::string worldId = std::to_string(i);
                nl::node worldScroll = scroll[worldId];
                if (worldScroll) {
                    std::cout << "  - scroll/" << worldId << " found" << std::endl;
                    nl::node frame0 = worldScroll["0"];
                    if (frame0) {
                        std::cout << "    - scroll/" << worldId << "/0 exists (type=" << (int)frame0.data_type() << ")" << std::endl;
                    }
                }
            }
        }
    }
    
    return 0;
}