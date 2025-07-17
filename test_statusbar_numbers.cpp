#include <iostream>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

int main() {
    std::cout << "Loading NX files..." << std::endl;
    
    nl::nx::load_all();
    
    std::cout << "\nChecking StatusBar number assets..." << std::endl;
    
    // Check v87 StatusBar structure
    nl::node statusBar = nl::nx::UI["StatusBar.img"];
    if (!statusBar.name().empty()) {
        std::cout << "\nV87 StatusBar.img found" << std::endl;
        
        // Check for number assets
        nl::node numbers = statusBar["gauge"]["number"];
        if (!numbers.name().empty()) {
            std::cout << "\nFound gauge/number node. Child nodes:" << std::endl;
            for (auto child : numbers) {
                std::string name = child.name();
                std::cout << "  Name: '" << name << "'" << std::endl;
                
                // Show ASCII value for special characters
                if (!name.empty()) {
                    char c = name[0];
                    std::cout << "    First char: '" << c << "' (ASCII: " << (int)c << ")" << std::endl;
                }
            }
        } else {
            std::cout << "No gauge/number node found" << std::endl;
        }
        
        // Also check base/number
        nl::node baseNumbers = statusBar["base"]["number"];
        if (!baseNumbers.name().empty()) {
            std::cout << "\nFound base/number node. Child nodes:" << std::endl;
            for (auto child : baseNumbers) {
                std::string name = child.name();
                std::cout << "  Name: '" << name << "'" << std::endl;
            }
        }
    }
    
    // Check modern StatusBar3 structure for comparison
    nl::node statusBar3 = nl::nx::UI["StatusBar3.img"];
    if (!statusBar3.name().empty()) {
        std::cout << "\n\nModern StatusBar3.img found" << std::endl;
        
        nl::node numbers = statusBar3["mainBar"]["status"]["number"];
        if (!numbers.name().empty()) {
            std::cout << "\nFound mainBar/status/number node. Child nodes:" << std::endl;
            for (auto child : numbers) {
                std::string name = child.name();
                std::cout << "  Name: '" << name << "'" << std::endl;
            }
        }
    }
    
    return 0;
}