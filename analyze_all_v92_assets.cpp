#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <json/json.h>
#include <filesystem>

namespace fs = std::filesystem;

// Structure to hold asset information
struct AssetInfo {
    std::string path;
    std::string type;
    int width = 0;
    int height = 0;
    bool hasNormal = false;
    bool hasMouseOver = false;
    bool hasPressed = false;
    bool hasDisabled = false;
};

class V92AssetAnalyzer {
private:
    std::map<std::string, std::vector<AssetInfo>> assetsByFile;
    std::set<std::string> missingAssets;
    
    void analyzeNode(const Json::Value& node, const std::string& path, std::vector<AssetInfo>& assets) {
        if (!node.isObject()) return;
        
        // Check if this is a canvas (actual image)
        if (node.isMember("_dirType") && node["_dirType"].asString() == "canvas") {
            AssetInfo info;
            info.path = path;
            info.type = "canvas";
            if (node.isMember("_width")) info.width = node["_width"].asInt();
            if (node.isMember("_height")) info.height = node["_height"].asInt();
            assets.push_back(info);
            return;
        }
        
        // Check if this is a button (has normal/mouseOver/pressed/disabled)
        bool isButton = node.isMember("normal") || node.isMember("mouseOver") || 
                       node.isMember("pressed") || node.isMember("disabled");
        
        if (isButton) {
            AssetInfo info;
            info.path = path;
            info.type = "button";
            info.hasNormal = node.isMember("normal");
            info.hasMouseOver = node.isMember("mouseOver");
            info.hasPressed = node.isMember("pressed");
            info.hasDisabled = node.isMember("disabled");
            assets.push_back(info);
        }
        
        // Recursively analyze children
        for (const auto& key : node.getMemberNames()) {
            if (key.empty() || key[0] == '_') continue;
            
            std::string newPath = path.empty() ? key : path + "/" + key;
            analyzeNode(node[key], newPath, assets);
        }
    }
    
public:
    void analyzeFile(const std::string& filename) {
        std::string fullPath = "C:\\HeavenClient\\ui-json\\UI.wz\\" + filename;
        std::ifstream file(fullPath);
        
        if (!file.is_open()) {
            std::cout << "Failed to open: " << filename << std::endl;
            return;
        }
        
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(file, root)) {
            std::cout << "Failed to parse JSON: " << filename << std::endl;
            return;
        }
        
        std::vector<AssetInfo> assets;
        analyzeNode(root, "", assets);
        assetsByFile[filename] = assets;
        
        file.close();
    }
    
    void checkExpectedAssets() {
        std::cout << "\n=== CHECKING EXPECTED ASSETS VS V92 ACTUAL ===\n" << std::endl;
        
        // Check Login.img assets
        std::cout << "LOGIN SCREEN ASSETS:" << std::endl;
        checkAsset("Login.img.json", "Common/frame");
        checkAsset("Login.img.json", "Common/BtStart");
        checkAsset("Login.img.json", "Common/BtStart2");
        checkAsset("Login.img.json", "Common/BtExit");
        checkAsset("Login.img.json", "Common/BtOk");
        checkAsset("Login.img.json", "Title/BtLogin");
        checkAsset("Login.img.json", "Title/BtQuit");
        checkAsset("Login.img.json", "Title/BtNew");
        checkAsset("Login.img.json", "Title/BtHomePage");
        checkAsset("Login.img.json", "Title/BtPasswdLost");
        checkAsset("Login.img.json", "Title/BtEmailLost");
        checkAsset("Login.img.json", "Title/BtEmailSave");
        checkAsset("Login.img.json", "Title/BtLoginIDSave");
        checkAsset("Login.img.json", "Common/BtHomePage");
        checkAsset("Login.img.json", "Common/BtPasswdLost");
        checkAsset("Login.img.json", "Common/BtEmailLost");
        checkAsset("Login.img.json", "Common/BtEmailSave");
        checkAsset("Login.img.json", "TabD");
        checkAsset("Login.img.json", "TabE");
        
        std::cout << "\nWORLD SELECT ASSETS:" << std::endl;
        checkAsset("Login.img.json", "Common/selectWorld");
        checkAsset("Login.img.json", "WorldSelect/BtWorld");
        checkAsset("Login.img.json", "WorldSelect/BtChannel");
        checkAsset("Login.img.json", "WorldSelect/chBackgrn");
        checkAsset("Login.img.json", "WorldSelect/BtNew");
        
        std::cout << "\nCHARACTER SELECT ASSETS:" << std::endl;
        checkAsset("Login.img.json", "CharSelect/BtNew");
        checkAsset("Login.img.json", "CharSelect/BtDelete");
        checkAsset("Login.img.json", "CharSelect/backgrnd");
        checkAsset("Login.img.json", "CharSelect/charInfo");
        
        std::cout << "\nBASIC UI ASSETS:" << std::endl;
        checkAsset("Basic.img.json", "BtClose");
        checkAsset("Basic.img.json", "BtClose3");
        
        std::cout << "\nUI WINDOW ASSETS:" << std::endl;
        checkAsset("UIWindow.img.json", "BtClose");
        checkAsset("UIWindow.img.json", "BtUIClose");
        checkAsset("UIWindow.img.json", "BtUIClose2");
        checkAsset("UIWindow.img.json", "Item/backgrnd");
        checkAsset("UIWindow.img.json", "Item/Tab/enabled");
        checkAsset("UIWindow.img.json", "Item/Tab/disabled");
        
        std::cout << "\nSTATUS BAR ASSETS:" << std::endl;
        checkAsset("StatusBar.img.json", "base/backgrnd");
        checkAsset("StatusBar.img.json", "base/backgrnd2");
        checkAsset("StatusBar.img.json", "gauge/bar");
        checkAsset("StatusBar.img.json", "gauge/graduation");
    }
    
    void checkAsset(const std::string& file, const std::string& path) {
        auto it = assetsByFile.find(file);
        if (it == assetsByFile.end()) {
            std::cout << "  ❌ " << path << " - FILE NOT LOADED" << std::endl;
            return;
        }
        
        bool found = false;
        for (const auto& asset : it->second) {
            if (asset.path == path) {
                found = true;
                std::cout << "  ✅ " << path;
                if (asset.type == "canvas") {
                    std::cout << " (canvas " << asset.width << "x" << asset.height << ")";
                } else if (asset.type == "button") {
                    std::cout << " (button";
                    if (asset.hasNormal) std::cout << " normal";
                    if (asset.hasMouseOver) std::cout << " mouseOver";
                    if (asset.hasPressed) std::cout << " pressed";
                    if (asset.hasDisabled) std::cout << " disabled";
                    std::cout << ")";
                }
                std::cout << std::endl;
                break;
            }
        }
        
        if (!found) {
            std::cout << "  ❌ " << path << " - NOT FOUND" << std::endl;
            missingAssets.insert(file + "/" + path);
        }
    }
    
    void printAllAssets() {
        std::cout << "\n=== ALL ASSETS IN V92 UI.WZ ===\n" << std::endl;
        
        for (const auto& [filename, assets] : assetsByFile) {
            std::cout << "\n" << filename << " (" << assets.size() << " assets):" << std::endl;
            for (const auto& asset : assets) {
                std::cout << "  " << asset.path;
                if (asset.type == "canvas") {
                    std::cout << " [" << asset.width << "x" << asset.height << "]";
                } else if (asset.type == "button") {
                    std::cout << " [BTN]";
                }
                std::cout << std::endl;
            }
        }
    }
    
    void suggestAlternatives() {
        std::cout << "\n=== SUGGESTED ALTERNATIVES FOR MISSING ASSETS ===\n" << std::endl;
        
        // Map missing assets to potential alternatives
        std::map<std::string, std::vector<std::string>> alternatives = {
            {"Login.img.json/Common/BtHomePage", {"Login.img.json/Title/BtHomePage"}},
            {"Login.img.json/Common/BtPasswdLost", {"Login.img.json/Title/BtLoginIDLost"}},
            {"Login.img.json/Common/BtEmailLost", {"Login.img.json/Title/BtLoginIDLost"}},
            {"Login.img.json/Common/BtEmailSave", {"Login.img.json/Title/BtLoginIDSave"}},
            {"Login.img.json/CharSelect/backgrnd", {"Map/Back/charselect.img", "Login.img.json/Common/frame"}},
            {"Login.img.json/WorldSelect/BtChannel", {"Login.img.json/Common/BtOk", "Login.img.json/Common/BtStart"}},
            {"Basic.img.json/BtClose", {"UIWindow.img.json/BtUIClose", "UIWindow.img.json/BtUIClose2"}}
        };
        
        for (const auto& missing : missingAssets) {
            std::cout << "Missing: " << missing << std::endl;
            if (alternatives.count(missing)) {
                std::cout << "  Alternatives:" << std::endl;
                for (const auto& alt : alternatives[missing]) {
                    std::cout << "    - " << alt << std::endl;
                }
            }
        }
    }
};

int main() {
    V92AssetAnalyzer analyzer;
    
    // List of all UI files
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
    
    std::cout << "=== V92 UI ASSET COMPREHENSIVE ANALYSIS ===" << std::endl;
    std::cout << "Analyzing " << uiFiles.size() << " UI files..." << std::endl;
    
    // Analyze each file
    for (const auto& file : uiFiles) {
        std::cout << "Loading " << file << "..." << std::endl;
        analyzer.analyzeFile(file);
    }
    
    // Check expected assets
    analyzer.checkExpectedAssets();
    
    // Print all assets
    analyzer.printAllAssets();
    
    // Suggest alternatives
    analyzer.suggestAlternatives();
    
    return 0;
}