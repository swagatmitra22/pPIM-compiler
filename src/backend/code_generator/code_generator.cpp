#include "backend/code_generator/code_generator.h"
#include "backend/simd/simd_generator.h"
#include "backend/memory_mapper/memory_mapper.h"
#include <iostream>
#include <iomanip>
#include <fstream>

namespace ppim {

CodeGenerator::CodeGenerator() : simdGenerator(std::make_unique<SIMDGenerator>()) {
    simdGenerator->initialize(4, 9); // 4 clusters per row, 9 cores per cluster
}

bool CodeGenerator::generatePIMCode(llvm::Module *module, std::vector<PIMInstruction> &instructions) {
    if (!module) {
        std::cerr << "Invalid module" << std::endl;
        return false;
    }
    
    MemoryMapper memMapper;
    
    for (auto &F : *module) {
        if (F.getName() == "matrix_multiply") {
            return generateMatrixMultiplicationCode(F, instructions, memMapper);
        }
    }
    
    std::cerr << "Matrix multiplication function not found" << std::endl;
    return false;
}

bool CodeGenerator::generateMatrixMultiplicationCode(llvm::Function &F, std::vector<PIMInstruction> &instructions, MemoryMapper &memMapper) {
    // Extract matrix dimensions and names from function arguments
    if (F.arg_size() != 6) {
        std::cerr << "Incorrect number of arguments for matrix multiplication" << std::endl;
        return false;
    }
    
    auto args = F.arg_begin();
    std::string matrixA = args++->getName();
    std::string matrixB = args++->getName();
    std::string resultMatrix = args++->getName();
    int rowsA = std::stoi(args++->getName());
    int colsA = std::stoi(args++->getName());
    int colsB = std::stoi(args++->getName());
    
    // Map matrices to memory
    memMapper.mapMatrix(matrixA, rowsA, colsA);
    memMapper.mapMatrix(matrixB, colsA, colsB);
    memMapper.mapMatrix(resultMatrix, rowsA, colsB);
    
    // Generate SIMD instructions for matrix multiplication
    auto simdInstructions = simdGenerator->generateMatrixMultSIMD(matrixA, matrixB, resultMatrix, memMapper);
    
    // Convert SIMD instructions to PIM instructions
    for (const auto &simdInst : simdInstructions) {
        instructions.push_back(convertToPIMInstruction(simdInst));
    }
    
    return true;
}

PIMInstruction CodeGenerator::convertToPIMInstruction(const SIMDInstruction &simdInst) {
    PIMInstruction pimInst;
    pimInst.type = static_cast<PIMInstructionType>(simdInst.type);
    pimInst.coreId = simdInst.coreId;
    pimInst.opcode = static_cast<PIMOpcode>(simdInst.opcode);
    pimInst.address = simdInst.address;
    return pimInst;
}

bool CodeGenerator::savePIMInstructions(const std::vector<PIMInstruction> &instructions, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    for (const auto &instr : instructions) {
        uint32_t encodedInstr = encodeInstruction(instr);
        file.write(reinterpret_cast<const char*>(&encodedInstr), 3); // Write 24 bits
    }
    
    file.close();
    return true;
}

uint32_t CodeGenerator::encodeInstruction(const PIMInstruction &instr) {
    uint32_t encodedInstr = 0;
    
    // Set instruction type (2 bits)
    encodedInstr |= (static_cast<uint32_t>(instr.type) & 0x3) << 22;
    
    // Set core ID or opcode (6 bits)
    if (instr.type == PIMInstructionType::PROG) {
        encodedInstr |= (instr.coreId & 0x3F) << 16;
    } else if (instr.type == PIMInstructionType::EXE) {
        encodedInstr |= (static_cast<uint32_t>(instr.opcode) & 0x3F) << 16;
    }
    
    // Set memory access bits and address
    if (instr.type == PIMInstructionType::MEMORY_READ) {
        encodedInstr |= 1 << 15;
    } else if (instr.type == PIMInstructionType::MEMORY_WRITE) {
        encodedInstr |= 1 << 14;
    }
    encodedInstr |= instr.address & 0x1FF;
    
    return encodedInstr;
}

void CodeGenerator::printPIMInstruction(const PIMInstruction &instr) {
    uint32_t encodedInstr = encodeInstruction(instr);
    std::cout << "Instruction: 0x" << std::hex << std::setw(6) << std::setfill('0') 
              << encodedInstr << std::dec << " (";
    
    switch (instr.type) {
        case PIMInstructionType::PROG:
            std::cout << "PROG, Core ID: " << static_cast<int>(instr.coreId);
            break;
        case PIMInstructionType::EXE:
            std::cout << "EXE, Opcode: " << static_cast<int>(instr.opcode);
            break;
        case PIMInstructionType::END:
            std::cout << "END";
            break;
        case PIMInstructionType::MEMORY_READ:
            std::cout << "MEMORY_READ, Address: 0x" << std::hex << instr.address << std::dec;
            break;
        case PIMInstructionType::MEMORY_WRITE:
            std::cout << "MEMORY_WRITE, Address: 0x" << std::hex << instr.address << std::dec;
            break;
    }
    
    std::cout << ")" << std::endl;
}

} // namespace ppim
