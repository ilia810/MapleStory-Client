// Simple console application to analyze StatusBar.img structure
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Include NX headers
#include "includes/NoLifeNx/nlnx/nx.hpp"
#include "includes/NoLifeNx/nlnx/node.hpp"

void analyzeStatusBar() {
    try {
        std::cout << "Loading NX files..." << std::endl;
        
        // Load NX files
        nl::nx::load_all();
        
        std::cout << "NX files loaded successfully." << std::endl;
        
        // Get StatusBar nodes
        nl::node statusBarV87 = nl::nx::UI["StatusBar.img"];
        nl::node statusBarModern = nl::nx::UI["StatusBar3.img"];
        
        std::cout << "Analyzing StatusBar structure..." << std::endl;
        
        // Create output file
        std::ofstream out("StatusBar_Analysis.txt");
        if (!out.is_open()) {
            std::cout << "Failed to create output file!" << std::endl;
            return;
        }
        
        out << "=== StatusBar.img Structure Analysis ===" << std::endl;
        out << "Generated for v87 asset mapping" << std::endl;
        out << std::endl;
        
        // Check v87 StatusBar
        out << "=== V87 StatusBar.img ===" << std::endl;
        if (statusBarV87.name().empty()) {
            out << "StatusBar.img NOT FOUND" << std::endl;
        } else {
            out << "StatusBar.img FOUND with " << statusBarV87.size() << " children" << std::endl;
            out << "Children:" << std::endl;
            
            for (const auto& child : statusBarV87) {
                out << "  " << child.name();
                if (child.size() > 0) {
                    out << " (folder with " << child.size() << " items)";
                } else {
                    out << " (leaf node)";
                }
                out << std::endl;
                
                // Show some important children
                if (child.name() == "base" || child.name() == "gauge") {
                    for (const auto& subchild : child) {
                        out << "    " << subchild.name() << std::endl;
                    }
                }
            }
        }
        
        out << std::endl;
        
        // Check modern StatusBar3
        out << "=== Modern StatusBar3.img ===" << std::endl;
        if (statusBarModern.name().empty()) {
            out << "StatusBar3.img NOT FOUND" << std::endl;
        } else {
            out << "StatusBar3.img FOUND with " << statusBarModern.size() << " children" << std::endl;
            out << "Children:" << std::endl;
            
            for (const auto& child : statusBarModern) {
                out << "  " << child.name();
                if (child.size() > 0) {
                    out << " (folder with " << child.size() << " items)";
                } else {
                    out << " (leaf node)";
                }
                out << std::endl;
            }
        }
        
        out << std::endl;
        out << "=== Key Findings ===" << std::endl;
        
        if (!statusBarV87.name().empty()) {
            out << "V87 StatusBar.img structure:" << std::endl;
            
            // Check base
            auto base = statusBarV87["base"];
            if (!base.name().empty()) {
                out << "  base/ exists with " << base.size() << " children:" << std::endl;
                for (const auto& child : base) {
                    out << "    " << child.name() << std::endl;
                }
            }
            
            // Check gauge
            auto gauge = statusBarV87["gauge"];
            if (!gauge.name().empty()) {
                out << "  gauge/ exists with " << gauge.size() << " children:" << std::endl;
                for (const auto& child : gauge) {
                    out << "    " << child.name() << std::endl;
                }
            }
            
            // Check buttons
            auto btMenu = statusBarV87["BtMenu"];
            out << "  BtMenu: " << (!btMenu.name().empty() ? "EXISTS" : "MISSING") << std::endl;
            
            auto btShop = statusBarV87["BtShop"];
            out << "  BtShop: " << (!btShop.name().empty() ? "EXISTS" : "MISSING") << std::endl;
            
            auto quickslot = statusBarV87["quickslot"];
            out << "  quickslot: " << (!quickslot.name().empty() ? "EXISTS" : "MISSING") << std::endl;
            
            auto number = statusBarV87["number"];
            out << "  number: " << (!number.name().empty() ? "EXISTS" : "MISSING") << std::endl;
        }
        
        out << std::endl;
        out << "=== Suggested Mappings ===" << std::endl;
        out << "For v87 compatibility in UIStatusBar.cpp:" << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/status/backgrnd" << std::endl;
        out << "  V87:    StatusBar.img/base/backgrnd" << std::endl;
        out << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/status/gauge/hp" << std::endl;
        out << "  V87:    StatusBar.img/gauge/hpFlash" << std::endl;
        out << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/status/gauge/mp" << std::endl;
        out << "  V87:    StatusBar.img/gauge/mpFlash" << std::endl;
        out << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/EXPBar/layer:gauge" << std::endl;
        out << "  V87:    StatusBar.img/gauge/exp" << std::endl;
        out << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/menu/button:Menu" << std::endl;
        out << "  V87:    StatusBar.img/BtMenu" << std::endl;
        out << std::endl;
        out << "  Modern: StatusBar3.img/mainBar/menu/button:CashShop" << std::endl;
        out << "  V87:    StatusBar.img/BtShop" << std::endl;
        
        out.close();
        
        std::cout << "Analysis complete! Check StatusBar_Analysis.txt for results." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "StatusBar.img Structure Analyzer" << std::endl;
    std::cout << "================================" << std::endl;
    
    analyzeStatusBar();
    
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}