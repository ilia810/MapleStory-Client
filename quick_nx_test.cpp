// Quick NX test - add this to your existing project temporarily
#include "MapleStory.h"
#include <iostream>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void quick_nx_explore() {
    std::cout << "=== QUICK NX EXPLORATION ===" << std::endl;
    
    try {
        // The main client should already have loaded NX files
        std::cout << "Checking if NX files are loaded..." << std::endl;
        
        // Check Login.img
        nl::node login = nl::nx::UI["Login.img"];
        std::cout << "Login.img exists: " << (login ? "YES" : "NO") << std::endl;
        
        if (login) {
            std::cout << "Login.img has " << login.size() << " children" << std::endl;
            std::cout << "First 10 children in Login.img:" << std::endl;
            int count = 0;
            for (const auto& child : login) {
                if (count >= 10) break;
                std::cout << "  " << child.name();
                if (child.data_type() == nl::node::type::none) {
                    std::cout << " [container, " << child.size() << " children]";
                } else if (child.data_type() == nl::node::type::bitmap) {
                    auto bmp = child.get_bitmap();
                    std::cout << " [bitmap " << bmp.width() << "x" << bmp.height() << "]";
                }
                std::cout << std::endl;
                count++;
            }
        }
        
        // Check for v83 indicators
        nl::node uiWindow = nl::nx::UI["UIWindow.img"];
        nl::node uiWindow2 = nl::nx::UI["UIWindow2.img"];
        
        std::cout << "\nVersion detection:" << std::endl;
        std::cout << "UIWindow.img exists: " << (uiWindow ? "YES (v83 mode)" : "NO") << std::endl;
        std::cout << "UIWindow2.img exists: " << (uiWindow2 ? "YES (modern mode)" : "NO") << std::endl;
        
        // Check specific button we're looking for
        if (login) {
            std::cout << "\nChecking for login buttons:" << std::endl;
            std::vector<std::string> buttonNames = {"BtLogin", "Login", "login", "btLogin"};
            for (const auto& name : buttonNames) {
                nl::node btn = login[name];
                if (btn) {
                    std::cout << "  " << name << " found: ";
                    if (btn.data_type() == nl::node::type::bitmap) {
                        auto bmp = btn.get_bitmap();
                        std::cout << "bitmap " << bmp.width() << "x" << bmp.height();
                    } else if (btn.data_type() == nl::node::type::none) {
                        std::cout << "container with " << btn.size() << " children";
                        if (btn.size() > 0) {
                            for (const auto& child : btn) {
                                if (child.data_type() == nl::node::type::bitmap) {
                                    auto bmp = child.get_bitmap();
                                    std::cout << " [" << child.name() << ": " << bmp.width() << "x" << bmp.height() << "]";
                                }
                            }
                        }
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        // Check background paths
        std::cout << "\nChecking background paths:" << std::endl;
        std::vector<std::string> bgPaths = {
            "Map/Obj/login.img/back/0",
            "Map/Back/login/0", 
            "UI/Login.img/Common/frame/0",
            "UI/Login.img/Common/frame/2"
        };
        
        for (const auto& path : bgPaths) {
            nl::node bg;
            if (path.substr(0, 3) == "Map") {
                // Parse Map path
                if (path.find("Map/Obj") == 0) {
                    bg = nl::nx::Map["Obj"]["login.img"]["back"]["0"];
                } else if (path.find("Map/Back") == 0) {
                    bg = nl::nx::Map["Back"]["login"]["0"];
                }
            } else if (path.substr(0, 2) == "UI") {
                // Parse UI path
                if (path.find("frame/0") != std::string::npos) {
                    bg = nl::nx::UI["Login.img"]["Common"]["frame"]["0"];
                } else if (path.find("frame/2") != std::string::npos) {
                    bg = nl::nx::UI["Login.img"]["Common"]["frame"]["2"];
                }
            }
            
            std::cout << "  " << path << ": " << (bg ? "FOUND" : "NOT FOUND");
            if (bg && bg.data_type() == nl::node::type::bitmap) {
                auto bmp = bg.get_bitmap();
                std::cout << " [" << bmp.width() << "x" << bmp.height() << "]";
            }
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== END EXPLORATION ===" << std::endl;
}