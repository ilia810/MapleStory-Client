#include <iostream>
#include <string>
#include <vector>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#include <fstream>
#include <sstream>
#include <json/json.h>

// Function to analyze UI asset paths from JSON files
void analyzeV92UIAssets() {
    std::cout << "=== V92 UI Asset Analysis ===" << std::endl << std::endl;
    
    // List of UI files to analyze
    std::vector<std::string> uiFiles = {
        "Basic.img.json",
        "BuffIcon.img.json", 
        "CashShop.img.json",
        "CashShopPreview.img.json",
        "ChatBalloon.img.json",
        "DialogImage.img.json",
        "GuildBBS.img.json",
        "GuildMark.img.json",
        "ITC.img.json",
        "ITCPreview.img.json",
        "Login.img.json",
        "Logo.img.json",
        "MapLogin.img.json",
        "MapLogin1.img.json",
        "MapleTV.img.json",
        "NameTag.img.json",
        "Npt_GuideLine.img.json",
        "OneADay.img.json",
        "StatusBar.img.json",
        "UIWindow.img.json",
        "tutorial.img.json"
    };
    
    // Common expected assets based on V83UIAssets.h
    std::cout << "1. LOGIN SCREEN ASSETS:" << std::endl;
    std::cout << "Client expects:" << std::endl;
    std::cout << "  - Login.img/Common/frame (background)" << std::endl;
    std::cout << "  - Login.img/Common/BtStart (login button)" << std::endl;
    std::cout << "  - Login.img/Common/BtExit (quit button)" << std::endl;
    std::cout << "  - Login.img/Title (title section)" << std::endl;
    std::cout << "  - Login.img/TabD, TabE (tabs)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "2. WORLD SELECT ASSETS:" << std::endl;
    std::cout << "Client expects:" << std::endl;
    std::cout << "  - Login.img/Common/selectWorld (background)" << std::endl;
    std::cout << "  - Login.img/WorldSelect/BtWorld/[0-22] (world buttons)" << std::endl;
    std::cout << "  - Login.img/WorldSelect/BtChannel/[0-19] (channel buttons)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "3. CHARACTER SELECT ASSETS:" << std::endl;
    std::cout << "Client expects:" << std::endl;
    std::cout << "  - Login.img/CharSelect/BtNew (new character button)" << std::endl;
    std::cout << "  - Login.img/CharSelect/backgrnd (background)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "4. INVENTORY ASSETS:" << std::endl;
    std::cout << "Client expects:" << std::endl;
    std::cout << "  - UIWindow.img/Item/backgrnd" << std::endl;
    std::cout << "  - UIWindow.img/Item/Tab/[enabled|disabled]/[0-4]" << std::endl;
    std::cout << std::endl;
    
    std::cout << "5. STATUS BAR ASSETS:" << std::endl;
    std::cout << "Client expects:" << std::endl;
    std::cout << "  - StatusBar.img/base/backgrnd" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Now let's check what v92 actually has..." << std::endl;
    std::cout << "=======================================" << std::endl << std::endl;
}

// Function to parse JSON and list all paths
void parseJsonFile(const std::string& filename) {
    std::string basePath = "C:\\HeavenClient\\ui-json\\UI.wz\\";
    std::string fullPath = basePath + filename;
    
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        std::cout << "Failed to open: " << filename << std::endl;
        return;
    }
    
    std::cout << "\n=== " << filename << " ===" << std::endl;
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(file, root)) {
        std::cout << "Failed to parse JSON: " << filename << std::endl;
        return;
    }
    
    // Recursively print structure
    std::function<void(const Json::Value&, const std::string&, int)> printStructure;
    printStructure = [&](const Json::Value& node, const std::string& path, int depth) {
        if (depth > 5) return; // Limit depth
        
        if (node.isObject()) {
            for (const auto& key : node.getMemberNames()) {
                if (key.empty() || key[0] == '_') continue; // Skip metadata
                
                std::string newPath = path.empty() ? key : path + "/" + key;
                
                // Print important paths
                if (key == "BtStart" || key == "BtLogin" || key == "BtExit" || 
                    key == "BtNew" || key == "BtWorld" || key == "BtChannel" ||
                    key == "frame" || key == "backgrnd" || key == "selectWorld" ||
                    key == "Common" || key == "CharSelect" || key == "WorldSelect" ||
                    key == "Title" || key == "Tab" || key == "base") {
                    std::cout << std::string(depth * 2, ' ') << newPath << std::endl;
                }
                
                printStructure(node[key], newPath, depth + 1);
            }
        }
    };
    
    printStructure(root, "", 0);
    file.close();
}

int main() {
    try {
        analyzeV92UIAssets();
        
        // Parse key files
        parseJsonFile("Login.img.json");
        parseJsonFile("UIWindow.img.json");
        parseJsonFile("StatusBar.img.json");
        parseJsonFile("Basic.img.json");
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}