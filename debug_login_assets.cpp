#include <iostream>
#include <string>
#include <vector>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void printNode(const nl::node& node, const std::string& indent = "") {
    std::cout << indent << node.name();
    
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
}

int main() {
    try {
        // Initialize NX files
        nl::nx::load_all();
        
        std::cout << "=== Login.img Asset Debug ===" << std::endl;
        
        nl::node login = nl::nx::UI["Login.img"];
        if (!login) {
            std::cout << "ERROR: Login.img not found!" << std::endl;
            return 1;
        }
        
        std::cout << "\nTop-level nodes in Login.img:" << std::endl;
        for (const auto& child : login) {
            printNode(child, "  ");
            
            // Check for button-like nodes
            if (child.name() == "Login" || child.name() == "BtLogin" || 
                child.name() == "New" || child.name() == "BtNew" ||
                child.name() == "Quit" || child.name() == "BtQuit") {
                std::cout << "    Found button: " << child.name() << std::endl;
                // Print button structure
                for (const auto& state : child) {
                    printNode(state, "      ");
                }
            }
        }
        
        // Specifically check for login button variations
        std::cout << "\n=== Checking Login Button Variations ===" << std::endl;
        std::vector<std::string> buttonNames = {
            "Login", "BtLogin", "login", "btLogin", "btlogin",
            "loginbutton", "LoginButton", "login_button"
        };
        
        for (const auto& name : buttonNames) {
            nl::node btn = login[name];
            if (btn) {
                std::cout << "Found: " << name << std::endl;
                for (const auto& state : btn) {
                    printNode(state, "  ");
                }
            }
        }
        
        // Check Common folder
        std::cout << "\n=== Checking Common folder ===" << std::endl;
        nl::node common = login["Common"];
        if (common) {
            std::cout << "Common folder found with " << common.size() << " children" << std::endl;
            for (const auto& child : common) {
                printNode(child, "  ");
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}