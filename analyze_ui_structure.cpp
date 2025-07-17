#include <iostream>
#include <string>
#include <vector>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void printNodeStructure(const nl::node& node, const std::string& path, int depth = 0) {
    if (depth > 3) return; // Limit depth to avoid too much output
    
    std::string indent(depth * 2, ' ');
    
    // Print current node info
    std::cout << indent << path << " [" << node.get_type_name() << "]";
    if (node.get_type() == nl::node::type::string) {
        std::cout << " = \"" << node.get_string() << "\"";
    } else if (node.get_type() == nl::node::type::integer) {
        std::cout << " = " << node.get_integer();
    }
    std::cout << std::endl;
    
    // Print children
    if (node.size() > 0 && depth < 3) {
        for (const auto& child : node) {
            printNodeStructure(child, path + "/" + child.name(), depth + 1);
        }
    }
}

void analyzeUIAssets() {
    try {
        std::cout << "=== Analyzing v83 UI.nx Structure ===" << std::endl;
        
        // Check Login.img structure
        std::cout << "\n--- Login.img Structure ---" << std::endl;
        auto login = nl::nx::UI["Login.img"];
        if (login) {
            // Check for key nodes
            std::vector<std::string> loginNodes = {
                "Title", "Common", "WorldSelect", "CharSelect", 
                "RaceSelect", "BtLogin", "BtQuit", "BtNew", "BtDelete"
            };
            
            for (const auto& nodeName : loginNodes) {
                auto node = login[nodeName];
                if (node) {
                    std::cout << "Found: Login.img/" << nodeName << std::endl;
                    // Check for specific sub-nodes
                    if (nodeName == "CharSelect") {
                        if (login["CharSelect"]["charSlot"]) 
                            std::cout << "  - Has charSlot" << std::endl;
                        if (login["CharSelect"]["charInfo1"]) 
                            std::cout << "  - Has charInfo1" << std::endl;
                        if (login["CharSelect"]["nameTag"]) 
                            std::cout << "  - Has nameTag" << std::endl;
                    }
                } else {
                    std::cout << "MISSING: Login.img/" << nodeName << std::endl;
                }
            }
        } else {
            std::cout << "ERROR: Login.img not found!" << std::endl;
        }
        
        // Check UIWindow.img structure (v83/v87)
        std::cout << "\n--- UIWindow.img Structure ---" << std::endl;
        auto uiwindow = nl::nx::UI["UIWindow.img"];
        if (uiwindow) {
            std::vector<std::string> windowNodes = {
                "Item", "Skill", "Equip", "Quest", "Stats"
            };
            
            for (const auto& nodeName : windowNodes) {
                auto node = uiwindow[nodeName];
                if (node) {
                    std::cout << "Found: UIWindow.img/" << nodeName << std::endl;
                    // Check Item specific nodes
                    if (nodeName == "Item") {
                        if (uiwindow["Item"]["backgrnd"]) 
                            std::cout << "  - Has backgrnd" << std::endl;
                        if (uiwindow["Item"]["Tab"]) 
                            std::cout << "  - Has Tab" << std::endl;
                        if (uiwindow["Item"]["New"]) 
                            std::cout << "  - Has New" << std::endl;
                    }
                } else {
                    std::cout << "MISSING: UIWindow.img/" << nodeName << std::endl;
                }
            }
        } else {
            std::cout << "NOTE: UIWindow.img not found (might be UIWindow2.img in newer versions)" << std::endl;
        }
        
        // Check UIWindow2.img structure (newer versions)
        std::cout << "\n--- UIWindow2.img Structure ---" << std::endl;
        auto uiwindow2 = nl::nx::UI["UIWindow2.img"];
        if (uiwindow2) {
            std::cout << "Found UIWindow2.img (newer version structure)" << std::endl;
        }
        
        // Check for Common UI elements
        std::cout << "\n--- Common UI Elements ---" << std::endl;
        auto common = nl::nx::UI["Login.img"]["Common"];
        if (common) {
            std::vector<std::string> commonNodes = {
                "frame", "BtStart", "BtPreview", "BtExit"
            };
            
            for (const auto& nodeName : commonNodes) {
                if (common[nodeName]) {
                    std::cout << "Found: Common/" << nodeName << std::endl;
                } else {
                    std::cout << "MISSING: Common/" << nodeName << std::endl;
                }
            }
        }
        
        // Print actual structure of Login.img to understand v83 layout
        std::cout << "\n--- Actual Login.img Structure (depth limited) ---" << std::endl;
        if (login) {
            printNodeStructure(login, "Login.img", 0);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing UI structure: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Initialize NX files
        nl::nx::load_all();
        
        analyzeUIAssets();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}