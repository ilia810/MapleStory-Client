// NX_STRUCTURE_EXPLORER.cpp
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>

void dump(const nl::node& n, const std::string& path = "", int depth = 0, int maxDepth = -1) {
    if (!n) return;
    if (maxDepth >= 0 && depth > maxDepth) return;
    
    const char* typeStr[] = {"none", "integer", "real", "string", "vector", "bitmap", "audio"};
    std::string fullPath = path.empty() ? n.name() : path + "/" + n.name();

    std::cout << std::left << std::setw(60) << fullPath
              << std::setw(10) << typeStr[static_cast<int>(n.data_type())];

    if (n.data_type() == nl::node::type::bitmap) {
        auto bmp = n.get_bitmap();
        std::cout << std::setw(12) << (std::to_string(bmp.width()) + "x" + std::to_string(bmp.height()));
        
        // Check for origin property
        nl::node parent = n.parent();
        if (parent) {
            nl::node origin = parent["origin"];
            if (origin) {
                int16_t x = origin["x"].get_integer();
                int16_t y = origin["y"].get_integer();
                std::cout << " origin=" << x << "," << y;
            }
        }
    } else if (n.data_type() == nl::node::type::none && n.size() > 0) {
        std::cout << std::setw(12) << (std::to_string(n.size()) + " children");
    } else if (n.data_type() == nl::node::type::integer) {
        std::cout << std::setw(12) << n.get_integer();
    } else if (n.data_type() == nl::node::type::string) {
        std::cout << std::setw(12) << n.get_string();
    } else if (n.data_type() == nl::node::type::vector) {
        auto v = n.get_vector();
        std::cout << std::setw(12) << ("(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")");
    }
    
    std::cout << "\n";

    // Recursively dump children
    for (const auto& child : n) {
        dump(child, fullPath, depth + 1, maxDepth);
    }
}

void dumpSpecificPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    
    if (parts.empty()) return;
    
    nl::node node;
    if (parts[0] == "UI") {
        node = nl::nx::UI;
    } else if (parts[0] == "Map") {
        node = nl::nx::Map;
    } else if (parts[0] == "Map001") {
        node = nl::nx::Map001;
    } else {
        std::cerr << "Unknown root: " << parts[0] << std::endl;
        return;
    }
    
    for (size_t i = 1; i < parts.size() && node; ++i) {
        node = node[parts[i]];
    }
    
    if (node) {
        dump(node, path);
    } else {
        std::cerr << "Path not found: " << path << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: NX_STRUCTURE_EXPLORER <command> [options]\n";
        std::cerr << "Commands:\n";
        std::cerr << "  dump <path>              - Dump specific path (e.g., UI/Login.img)\n";
        std::cerr << "  login                    - Dump all login-related assets\n";
        std::cerr << "  inventory                - Dump inventory-related assets\n";
        std::cerr << "  --max-depth N            - Limit depth\n";
        return 1;
    }
    
    // Initialize NX (assuming Data.nx is in current directory or ../nx_files)
    try {
        nl::nx::load_all("../nx_files");
    } catch (...) {
        try {
            nl::nx::load_all("./nx_files");
        } catch (...) {
            try {
                nl::nx::load_all("../Data");
            } catch (...) {
                std::cerr << "Failed to load NX files. Make sure nx_files directory exists.\n";
                return 1;
            }
        }
    }
    
    std::string command = argv[1];
    int maxDepth = -1;
    
    // Parse max depth if provided
    for (int i = 2; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--max-depth") {
            maxDepth = std::stoi(argv[i + 1]);
        }
    }
    
    if (command == "dump" && argc >= 3) {
        dumpSpecificPath(argv[2]);
    } else if (command == "login") {
        std::cout << "=== UI/Login.img ===" << std::endl;
        dumpSpecificPath("UI/Login.img");
        
        std::cout << "\n=== Map/Obj/login.img ===" << std::endl;
        dumpSpecificPath("Map/Obj/login.img");
        
        std::cout << "\n=== Map/Back/login ===" << std::endl;
        dumpSpecificPath("Map/Back/login");
    } else if (command == "inventory") {
        std::cout << "=== UI/UIWindow.img/Item ===" << std::endl;
        dumpSpecificPath("UI/UIWindow.img/Item");
        
        std::cout << "\n=== UI/UIWindow2.img/Item ===" << std::endl;
        dumpSpecificPath("UI/UIWindow2.img/Item");
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }
    
    return 0;
}