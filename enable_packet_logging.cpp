// ENABLE PACKET LOGGING FOR MOB SPAWN DEBUGGING

#include <iostream>
#include "Net/PacketSwitch.h"
#include "Net/Session.h"
#include "Configuration.h"

/*
To enable packet logging in the existing code:

1. The PacketSwitch.cpp already has logging code that can be enabled
2. Look for the variable "log_packets" in PacketSwitch.cpp
3. It logs SPAWN_MOB (236) and SPAWN_MOB_C (238) packets
4. The logging is limited to first 50 packets (packet_count < 50)

To enable it, you need to set log_packets = true in PacketSwitch.cpp
Or add this to Configuration to enable it at runtime.
*/

void enablePacketLogging() {
    std::cout << "=== ENABLING PACKET LOGGING ===" << std::endl;
    
    // Check if Configuration has packet logging option
    Configuration& config = Configuration::get();
    
    // The packet logging is controlled by "log_packets" variable in PacketSwitch.cpp
    // You can manually enable it by changing this line in PacketSwitch.cpp:
    // bool log_packets = false;  // CHANGE TO: bool log_packets = true;
    
    std::cout << "To enable packet logging:" << std::endl;
    std::cout << "1. Open Net/PacketSwitch.cpp" << std::endl;
    std::cout << "2. Find the line: bool log_packets = false;" << std::endl;
    std::cout << "3. Change it to: bool log_packets = true;" << std::endl;
    std::cout << "4. Recompile the client" << std::endl;
    std::cout << "5. Run the client and watch console output" << std::endl;
    
    std::cout << "\nWhat you'll see if SPAWN_MOB packets are received:" << std::endl;
    std::cout << "- Console output: 'Packet: SPAWN_MOB (236)'" << std::endl;
    std::cout << "- Console output: 'Packet: SPAWN_MOB_C (238)'" << std::endl;
    
    std::cout << "\nIf you don't see these messages:" << std::endl;
    std::cout << "- Server is not sending spawn packets for current map" << std::endl;
    std::cout << "- Map has no mobs configured" << std::endl;
    std::cout << "- Server configuration issue" << std::endl;
}

void checkCurrentSettings() {
    std::cout << "\n=== CURRENT SETTINGS CHECK ===" << std::endl;
    
    Configuration& config = Configuration::get();
    
    std::cout << "Show packets: " << (config.get_show_packets() ? "YES" : "NO") << std::endl;
    
    if (config.get_show_packets()) {
        std::cout << "Packet logging is enabled in configuration." << std::endl;
    } else {
        std::cout << "Packet logging is disabled in configuration." << std::endl;
        std::cout << "Note: PacketSwitch has its own log_packets variable." << std::endl;
    }
}

void suggestMapTest() {
    std::cout << "\n=== SUGGESTED MAP TEST ===" << std::endl;
    
    std::cout << "To test mob spawning, try these maps:" << std::endl;
    std::cout << "1. Henesys (100000000) - Should have Orange Mushrooms" << std::endl;
    std::cout << "2. Ellinia (101000000) - Should have various mobs" << std::endl;
    std::cout << "3. Perion (102000000) - Should have different mobs" << std::endl;
    
    std::cout << "\nTest procedure:" << std::endl;
    std::cout << "1. Enable packet logging (see above)" << std::endl;
    std::cout << "2. Connect to server" << std::endl;
    std::cout << "3. Load one of the test maps" << std::endl;
    std::cout << "4. Wait 1-2 minutes for server to send spawn packets" << std::endl;
    std::cout << "5. Look for SPAWN_MOB messages in console" << std::endl;
    
    std::cout << "\nIf no packets are received:" << std::endl;
    std::cout << "- Check server console/logs for errors" << std::endl;
    std::cout << "- Verify server has mob spawn data for the map" << std::endl;
    std::cout << "- Check if server is configured to spawn mobs automatically" << std::endl;
}

int main() {
    std::cout << "Mob Spawn Packet Logging Guide" << std::endl;
    std::cout << "==============================" << std::endl;
    
    enablePacketLogging();
    checkCurrentSettings();
    suggestMapTest();
    
    std::cout << "\nPress any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}