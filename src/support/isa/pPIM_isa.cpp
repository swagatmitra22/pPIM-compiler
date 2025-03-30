#include "support/isa/pPIM_isa.h"
#include <sstream>
#include <iomanip>

namespace ppim {

uint32_t encodeInstruction(const Instruction& inst) {
    // Encode the 24-bit instruction into a 32-bit word
    uint32_t encoded = 0;
    
    // Set the instruction type (2 bits)
    encoded |= (inst.opcode & 0x3) << 22;
    
    // Set the core ID (6 bits)
    encoded |= (inst.coreId & 0x3F) << 16;
    
    // Set the read/write bits
    encoded |= (inst.readBit & 0x1) << 15;
    encoded |= (inst.writeBit & 0x1) << 14;
    
    // Set the row address (9 bits)
    encoded |= (inst.rowAddress & 0x1FF);
    
    return encoded;
}

Instruction decodeInstruction(uint32_t encoded) {
    Instruction inst;
    
    // Extract the instruction type (2 bits)
    inst.opcode = (encoded >> 22) & 0x3;
    
    // Extract the core ID (6 bits)
    inst.coreId = (encoded >> 16) & 0x3F;
    
    // Extract the read/write bits
    inst.readBit = (encoded >> 15) & 0x1;
    inst.writeBit = (encoded >> 14) & 0x1;
    
    // Extract the row address (9 bits)
    inst.rowAddress = encoded & 0x1FF;
    
    return inst;
}

ControlWord generateControlWordForAdd() {
    ControlWord cw;
    
    // In a real implementation, this would set the appropriate bits
    // to configure the LUTs for addition operations
    
    // For now, we'll just set some placeholder values
    cw.data[0] = 0xAA; // Addition operation code
    
    return cw;
}

ControlWord generateControlWordForMultiply() {
    ControlWord cw;
    
    // In a real implementation, this would set the appropriate bits
    // to configure the LUTs for multiplication operations
    
    // For now, we'll just set some placeholder values
    cw.data[0] = 0xBB; // Multiplication operation code
    
    return cw;
}

ControlWord generateControlWordForMAC() {
    ControlWord cw;
    
    // In a real implementation, this would set the appropriate bits
    // to configure the LUTs for MAC operations as shown in Fig. 6
    
    // For now, we'll just set some placeholder values
    cw.data[0] = 0xCC; // MAC operation code
    
    return cw;
}

std::string instructionToString(const Instruction& inst) {
    std::stringstream ss;
    
    // Determine the instruction type
    std::string typeStr;
    switch (inst.opcode) {
        case 0: typeStr = "MEMORY"; break;
        case 1: typeStr = "PROG"; break;
        case 2: typeStr = "EXE"; break;
        case 3: typeStr = "END"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    
    ss << "Type: " << typeStr << ", ";
    
    if (inst.opcode == 0) {
        // Memory access instruction
        ss << "R/W: " << inst.readBit << "/" << inst.writeBit << ", ";
        ss << "Row: 0x" << std::hex << std::setw(3) << std::setfill('0') << inst.rowAddress;
    } else if (inst.opcode == 1) {
        // LUT programming instruction
        ss << "Core: " << static_cast<int>(inst.coreId);
    }
    
    return ss.str();
}

std::string controlWordToString(const ControlWord& cw) {
    std::stringstream ss;
    
    ss << "Control Word: ";
    for (int i = 0; i < 15; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(cw.data[i]) << " ";
    }
    
    return ss.str();
}

} // namespace ppim
