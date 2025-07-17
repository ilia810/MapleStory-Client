#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Include the necessary headers
#define USE_NX
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void dumpStatusBarStructure()
{
    try
    {
        // Check both v87 and modern paths
        nl::node statusBarV87 = nl::nx::UI["StatusBar.img"];
        nl::node statusBarModern = nl::nx::UI["StatusBar3.img"];
        
        std::ofstream outFile("StatusBar_structure_analysis.txt");
        outFile << "=== StatusBar.img Structure Analysis ===" << std::endl;
        outFile << "Generated to understand v87 asset structure" << std::endl;
        outFile << std::endl;
        
        // Function to recursively dump node structure
        auto dumpNode = [&](const nl::node& node, const std::string& path, int depth, auto& self) -> void {
            if (depth > 6) return; // Prevent infinite recursion
            
            std::string indent(depth * 2, ' ');
            std::string nodeName = node.name();
            
            // Show node type
            std::string nodeType = "unknown";
            std::string nodeValue = "";
            
            if (node.data_type() == nl::node::type::bitmap) {
                nodeType = "bitmap";
            } else if (node.data_type() == nl::node::type::string) {
                nodeType = "string";
                try {
                    nodeValue = " = \"" + node.get_string() + "\"";
                } catch (...) {}
            } else if (node.data_type() == nl::node::type::integer) {
                nodeType = "integer";
                try {
                    nodeValue = " = " + std::to_string(node.get_integer());
                } catch (...) {}
            } else if (node.data_type() == nl::node::type::real) {
                nodeType = "real";
                try {
                    nodeValue = " = " + std::to_string(node.get_real());
                } catch (...) {}
            } else if (node.data_type() == nl::node::type::vector) {
                nodeType = "vector";
                try {
                    auto vec = node.get_vector();
                    nodeValue = " = (" + std::to_string(vec.first) + ", " + std::to_string(vec.second) + ")";
                } catch (...) {}
            } else if (node.size() > 0) {
                nodeType = "folder";
            }
            
            outFile << indent << path << " [" << nodeType << "]" << nodeValue;
            if (node.size() > 0) {
                outFile << " (" << node.size() << " children)";
            }
            outFile << std::endl;
            
            // Recursively process children
            for (const auto& child : node) {
                self(child, path + "/" + child.name(), depth + 1, self);
            }
        };
        
        // Analyze v87 StatusBar.img
        outFile << "=== V87 StatusBar.img Structure ===" << std::endl;
        if (!statusBarV87.name().empty()) {
            outFile << "StatusBar.img found! Root has " << statusBarV87.size() << " children" << std::endl;
            outFile << std::endl;
            
            dumpNode(statusBarV87, "StatusBar.img", 0, dumpNode);
        } else {
            outFile << "StatusBar.img NOT FOUND" << std::endl;
        }
        
        outFile << std::endl;
        outFile << "=== Modern StatusBar3.img Structure ===" << std::endl;
        if (!statusBarModern.name().empty()) {
            outFile << "StatusBar3.img found! Root has " << statusBarModern.size() << " children" << std::endl;
            outFile << std::endl;
            
            dumpNode(statusBarModern, "StatusBar3.img", 0, dumpNode);
        } else {
            outFile << "StatusBar3.img NOT FOUND" << std::endl;
        }
        
        outFile << std::endl;
        outFile << "=== Key Nodes Analysis ===" << std::endl;
        
        // Check specific nodes that the modern client expects
        if (!statusBarV87.name().empty()) {
            outFile << "V87 Key Nodes:" << std::endl;
            
            // Check base node
            nl::node base = statusBarV87["base"];
            outFile << "  base: " << (!base.name().empty() ? "EXISTS" : "MISSING") << std::endl;
            if (!base.name().empty()) {
                outFile << "    base children: " << base.size() << std::endl;
                for (const auto& child : base) {
                    outFile << "      " << child.name() << " [" << (child.data_type() == nl::node::type::bitmap ? "bitmap" : "other") << "]" << std::endl;
                }
            }
            
            // Check gauge node
            nl::node gauge = statusBarV87["gauge"];
            outFile << "  gauge: " << (!gauge.name().empty() ? "EXISTS" : "MISSING") << std::endl;
            if (!gauge.name().empty()) {
                outFile << "    gauge children: " << gauge.size() << std::endl;
                for (const auto& child : gauge) {
                    outFile << "      " << child.name() << " [" << (child.data_type() == nl::node::type::bitmap ? "bitmap" : "other") << "]" << std::endl;
                }
            }
            
            // Check other important nodes
            std::vector<std::string> importantNodes = {"BtMenu", "BtShop", "quickslot", "number"};
            for (const std::string& nodeName : importantNodes) {
                nl::node node = statusBarV87[nodeName];
                outFile << "  " << nodeName << ": " << (!node.name().empty() ? "EXISTS" : "MISSING") << std::endl;
                if (!node.name().empty() && node.size() > 0) {
                    outFile << "    " << nodeName << " children: " << node.size() << std::endl;
                    for (const auto& child : node) {
                        outFile << "      " << child.name() << " [" << (child.data_type() == nl::node::type::bitmap ? "bitmap" : "other") << "]" << std::endl;
                    }
                }
            }
        }
        
        outFile << std::endl;
        outFile << "=== Modern Client Expected Paths ===" << std::endl;
        outFile << "Modern client expects these paths in StatusBar3.img:" << std::endl;
        outFile << "  mainBar/status/backgrnd" << std::endl;
        outFile << "  mainBar/status/layer:cover" << std::endl;
        outFile << "  mainBar/status/layer:Lv" << std::endl;
        outFile << "  mainBar/status/gauge/hp/layer:0" << std::endl;
        outFile << "  mainBar/status/gauge/mp/layer:0" << std::endl;
        outFile << "  mainBar/EXPBar/backgrnd" << std::endl;
        outFile << "  mainBar/EXPBar/layer:gauge" << std::endl;
        outFile << "  mainBar/menu/button:*" << std::endl;
        outFile << "  mainBar/quickSlot/*" << std::endl;
        outFile << std::endl;
        
        outFile << "=== Mapping Recommendations ===" << std::endl;
        outFile << "Based on structure analysis, these mappings might work:" << std::endl;
        if (!statusBarV87.name().empty()) {
            outFile << "  StatusBar3.img/mainBar/status/backgrnd -> StatusBar.img/base/backgrnd" << std::endl;
            outFile << "  StatusBar3.img/mainBar/status/gauge/hp -> StatusBar.img/gauge/hpFlash" << std::endl;
            outFile << "  StatusBar3.img/mainBar/status/gauge/mp -> StatusBar.img/gauge/mpFlash" << std::endl;
            outFile << "  StatusBar3.img/mainBar/EXPBar/layer:gauge -> StatusBar.img/gauge/exp" << std::endl;
            outFile << "  StatusBar3.img/mainBar/menu/button:Menu -> StatusBar.img/BtMenu" << std::endl;
            outFile << "  StatusBar3.img/mainBar/menu/button:CashShop -> StatusBar.img/BtShop" << std::endl;
        }
        
        outFile.close();
        
        // Also output to console for immediate viewing
        std::cout << "StatusBar structure analysis complete! Check StatusBar_structure_analysis.txt" << std::endl;
        
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error analyzing StatusBar structure: " << ex.what() << std::endl;
    }
}

int main()
{
    try
    {
        // Load NX files
        nl::nx::load_all();
        
        // Dump the structure
        dumpStatusBarStructure();
        
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
        std::cin.get();
    }
    
    return 0;
}