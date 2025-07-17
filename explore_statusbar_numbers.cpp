#include <iostream>
#include <string>
#include <iomanip>
#include "Util/NxFiles.h"
#include "Util/Misc.h"
#include "Constants.h"

int main()
{
    try {
        // Initialize NX files
        ms::NxFiles::init();
        
        std::cout << "Exploring StatusBar.img number nodes..." << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Access the StatusBar.img node
        nl::node statusBar = ms::UI["StatusBar.img"];
        
        if (!statusBar) {
            std::cout << "Error: Could not find StatusBar.img" << std::endl;
            return 1;
        }
        
        // Look for number-related nodes
        std::vector<std::string> numberPaths = {
            "number",
            "statusbar3/number",
            "statusbar2/number",
            "v83/number",
            "v87/number"
        };
        
        for (const std::string& path : numberPaths) {
            nl::node numbers = statusBar[path];
            
            if (numbers && numbers.data_type() != nl::node::type::none) {
                std::cout << "\nFound numbers at: " << path << std::endl;
                std::cout << "Node type: " << static_cast<int>(numbers.data_type()) << std::endl;
                std::cout << "Children:" << std::endl;
                
                // List all child nodes
                for (nl::node child : numbers) {
                    std::string name = child.name();
                    std::cout << "  - \"" << name << "\"";
                    
                    // Show the character it represents
                    if (!name.empty()) {
                        char c = name[0];
                        if (c == '\\') c = '/';
                        std::cout << " (char: '" << c << "', ASCII: " << static_cast<int>(c) << ")";
                    }
                    
                    // Show node type
                    std::cout << " [type: " << static_cast<int>(child.data_type()) << "]";
                    
                    if (child.data_type() == nl::node::type::bitmap) {
                        std::cout << " [bitmap]";
                    }
                    
                    std::cout << std::endl;
                }
            }
        }
        
        // Also check the main statusbar node for number
        nl::node mainNumbers = statusBar["number"];
        if (mainNumbers && mainNumbers.data_type() != nl::node::type::none) {
            std::cout << "\nMain StatusBar number nodes:" << std::endl;
            
            // List special character nodes
            std::vector<std::string> specialNames = {
                "Lbracket", "Rbracket", "slash", "percent", "colon", "dot", "comma",
                "(", ")", "[", "]", "/", "%", ":", ".", ","
            };
            
            for (const std::string& name : specialNames) {
                nl::node node = mainNumbers[name];
                if (node && node.data_type() == nl::node::type::bitmap) {
                    std::cout << "  Found special: \"" << name << "\"" << std::endl;
                }
            }
            
            // Check for number nodes (0-9)
            for (int i = 0; i <= 9; ++i) {
                std::string numStr = std::to_string(i);
                nl::node node = mainNumbers[numStr];
                if (node && node.data_type() == nl::node::type::bitmap) {
                    std::cout << "  Found number: \"" << numStr << "\"" << std::endl;
                }
            }
        }
        
        std::cout << "\nDone!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}