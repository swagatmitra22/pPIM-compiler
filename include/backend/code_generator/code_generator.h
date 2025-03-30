#ifndef PPIM_CODE_GENERATOR_H
#define PPIM_CODE_GENERATOR_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"

namespace ppim {

// Instruction types
enum class PIMInstructionType {
    PROG,           // Program a core
    EXE,            // Execute an operation
    END,            // End an operation
    MEMORY_READ,    // Read from memory
    MEMORY_WRITE    // Write to memory
};

// Operation codes
enum class PIMOpcode {
    MULTIPLY,
    ADD,
    MAC,
    RELU
};

// Instruction structure
struct PIMInstruction {
    PIMInstructionType type;
    uint8_t coreId;       // Core ID for PROG instructions
    PIMOpcode opcode;     // Operation code for EXE instructions
    uint32_t address;     // Memory address for MEMORY_READ/WRITE instructions
    
    PIMInstruction() : type(PIMInstructionType::END), coreId(0), opcode(PIMOpcode::ADD), address(0) {}
};

// Code generator class
class CodeGenerator {
public:
    CodeGenerator();
    
    // Generate pPIM instructions from LLVM IR
    bool generatePIMCode(llvm::Module *module, std::vector<PIMInstruction> &instructions);
    
    // Save pPIM instructions to a file
    bool savePIMInstructions(const std::vector<PIMInstruction> &instructions, const std::string &filename);
    
    // Print a pPIM instruction
    void printPIMInstruction(const PIMInstruction &instr);
    
private:
    // Generate pPIM instructions for a single LLVM instruction
    std::vector<PIMInstruction> generateInstructionsForLLVMInst(llvm::Instruction *inst);
    
    // Get the memory address for a pointer
    uint32_t getMemoryAddress(llvm::Value *ptr);
    
    // Encode a pPIM instruction into a 24-bit value
    uint32_t encodeInstruction(const PIMInstruction &instr);
};

} // namespace ppim

#endif // PPIM_CODE_GENERATOR_H
