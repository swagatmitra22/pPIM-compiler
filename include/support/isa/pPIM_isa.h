#ifndef PPIM_ISA_H
#define PPIM_ISA_H

#include <cstdint>
#include <string>

namespace ppim {

// Instruction types as described in the ISA slides
enum class InstructionType {
    MEMORY_ACCESS,  // Memory access instructions
    LUT_PROGRAMMING, // LUT programming instructions (PROG)
    COMPUTE         // Compute instructions (EXE, END)
};

// Instruction format: 24-bit fixed-length
// - 2 bits: Instruction type (00: Memory, 01: PROG, 10: EXE, 11: END)
// - 6 bits: Core pointer/ID (for LUT programming)
// - 1 bit: Read bit (for memory access)
// - 1 bit: Write bit (for memory access)
// - 9 bits: Row address (for memory access)
// - Remaining bits: Reserved or operation-specific

// Control word format: 120-bit
// This is used for LUT programming
struct ControlWord {
    uint8_t data[15]; // 120 bits = 15 bytes
    
    ControlWord() {
        for (int i = 0; i < 15; i++) {
            data[i] = 0;
        }
    }
};

// Instruction format
struct Instruction {
    uint32_t opcode : 2;     // Instruction type
    uint32_t coreId : 6;     // Core pointer/ID
    uint32_t readBit : 1;    // Read bit
    uint32_t writeBit : 1;   // Write bit
    uint32_t rowAddress : 9; // Row address
    uint32_t reserved : 5;   // Reserved bits
    
    Instruction() : opcode(0), coreId(0), readBit(0), writeBit(0), rowAddress(0), reserved(0) {}
};

// Convert between 24-bit instruction and 32-bit representation
uint32_t encodeInstruction(const Instruction& inst);
Instruction decodeInstruction(uint32_t encoded);

// Generate control words for different operations
ControlWord generateControlWordForAdd();
ControlWord generateControlWordForMultiply();
ControlWord generateControlWordForMAC();

// Utility functions
std::string instructionToString(const Instruction& inst);
std::string controlWordToString(const ControlWord& cw);

} // namespace ppim

#endif // PPIM_ISA_H
