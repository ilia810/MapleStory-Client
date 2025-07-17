// PACKET LOGGING PATCH
// Add this code to PacketSwitch.cpp to log SPAWN_MOB packets

// In PacketSwitch.cpp, find the handle() method and add logging:

/*
BEFORE (around line in PacketSwitch.cpp):
void PacketSwitch::handle(InPacket& recv) const
{
    uint16_t opcode = recv.read_short();
    
    // ... existing code ...
}

AFTER:
void PacketSwitch::handle(InPacket& recv) const
{
    uint16_t opcode = recv.read_short();
    
    // ADD THIS LOGGING CODE:
    if (opcode == 236 || opcode == 238) {  // SPAWN_MOB or SPAWN_MOB_C
        std::cout << "*** SPAWN_MOB PACKET RECEIVED ***" << std::endl;
        std::cout << "Opcode: " << opcode << std::endl;
        std::cout << "Packet size: " << recv.available() << " bytes" << std::endl;
        
        // Log to file as well
        std::ofstream logFile("mob_spawn_packets.log", std::ios::app);
        logFile << "Opcode " << opcode << " received at " << std::time(nullptr) << std::endl;
        logFile.close();
    }
    
    // ... rest of existing code ...
}
*/

// OR, add logging to the SpawnMobHandler itself:
/*
In MapObjectHandlers.cpp, modify SpawnMobHandler::handle():

void SpawnMobHandler::handle(InPacket& recv) const
{
    std::cout << "*** SPAWN_MOB HANDLER CALLED ***" << std::endl;
    std::cout << "Packet size: " << recv.available() << " bytes" << std::endl;
    
    int32_t oid = recv.read_int();
    recv.read_byte(); // 5 if controller == null
    int32_t id = recv.read_int();
    
    std::cout << "Spawning mob: OID=" << oid << ", ID=" << id << std::endl;
    
    // ... rest of existing code ...
    
    std::cout << "Mob spawn completed." << std::endl;
}
*/

// Quick test to check if packets are being processed:
void testPacketReception() {
    std::cout << "Checking if PacketSwitch is receiving packets..." << std::endl;
    
    // This would need to be added to the main game loop
    Session& session = Session::get();
    
    for (int i = 0; i < 100; i++) {
        session.update();  // This processes incoming packets
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Packet reception test completed." << std::endl;
}