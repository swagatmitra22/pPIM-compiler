#include "backend/simd/simd_generator.h"
#include <iostream>
#include <algorithm>

namespace ppim {

SIMDGenerator::SIMDGenerator() 
    : ClustersPerRow(4), CoresPerCluster(9) {
    // Default initialization with typical pPIM architecture parameters
    // 4 clusters per row, 9 cores per cluster as described in the slides
}

void SIMDGenerator::initialize(uint32_t clustersPerRow, uint32_t coresPerCluster) {
    ClustersPerRow = clustersPerRow;
    CoresPerCluster = coresPerCluster;
}

std::vector<PIMInstruction> SIMDGenerator::generateMatrixMultSIMD(
    const std::string &matrixA, const std::string &matrixB, 
    const std::string &resultMatrix, const MemoryMapper &memMapper) {
    
    std::vector<PIMInstruction> instructions;
    
    // Get matrix layouts
    MatrixMemoryLayout layoutA = memMapper.getMatrixLayout(matrixA);
    MatrixMemoryLayout layoutB = memMapper.getMatrixLayout(matrixB);
    MatrixMemoryLayout layoutC = memMapper.getMatrixLayout(resultMatrix);
    
    // Check if matrices can be multiplied
    if (layoutA.cols != layoutB.rows) {
        std::cerr << "Error: Matrix dimensions do not match for multiplication" << std::endl;
        return instructions;
    }
    
    // Generate SIMD LUT programming instructions for MAC operation
    auto progInstructions = generateSIMDLUTProgramming(PIMOpcode::MAC);
    instructions.insert(instructions.end(), progInstructions.begin(), progInstructions.end());
    
    // For each row of matrix A
    for (uint32_t i = 0; i < layoutA.rows; i++) {
        // For each column of matrix B
        for (uint32_t j = 0; j < layoutB.cols; j++) {
            // Generate memory access instructions to load data from matrix A and B
            std::vector<uint32_t> readAddresses;
            
            // Get addresses for row i of matrix A
            for (uint32_t k = 0; k < layoutA.cols; k++) {
                PhysicalMemoryLocation loc = memMapper.getElementLocation(layoutA, i, k);
                readAddresses.push_back(loc.rowAddress);
            }
            
            // Get addresses for column j of matrix B
            for (uint32_t k = 0; k < layoutB.rows; k++) {
                PhysicalMemoryLocation loc = memMapper.getElementLocation(layoutB, k, j);
                readAddresses.push_back(loc.rowAddress);
            }
            
            // Generate SIMD memory read instructions
            auto readInstructions = generateSIMDMemoryAccess(true, readAddresses);
            instructions.insert(instructions.end(), readInstructions.begin(), readInstructions.end());
            
            // Generate SIMD compute instructions for MAC operation
            auto computeInstructions = generateSIMDCompute(PIMOpcode::MAC);
            instructions.insert(instructions.end(), computeInstructions.begin(), computeInstructions.end());
            
            // Generate memory write instruction for the result
            PhysicalMemoryLocation resultLoc = memMapper.getElementLocation(layoutC, i, j);
            std::vector<uint32_t> writeAddresses = {resultLoc.rowAddress};
            auto writeInstructions = generateSIMDMemoryAccess(false, writeAddresses);
            instructions.insert(instructions.end(), writeInstructions.begin(), writeInstructions.end());
        }
    }
    
    return instructions;
}

std::vector<PIMInstruction> SIMDGenerator::generateAtomicInstructions(
    PIMOpcode opcode, uint32_t numOperations) {
    
    std::vector<PIMInstruction> instructions;
    
    // Generate a single set of LUT programming instructions
    auto progInstructions = generateSIMDLUTProgramming(opcode);
    instructions.insert(instructions.end(), progInstructions.begin(), progInstructions.end());
    
    // Generate a single compute instruction that will be executed multiple times
    PIMInstruction exeInst;
    exeInst.type = PIMInstructionType::EXE;
    exeInst.opcode = opcode;
    // In a real implementation, we would set a repeat count or similar field
    instructions.push_back(exeInst);
    
    // Generate END instruction
    PIMInstruction endInst;
    endInst.type = PIMInstructionType::END;
    instructions.push_back(endInst);
    
    return instructions;
}

std::vector<PIMInstruction> SIMDGenerator::mapToSIMD(
    const std::vector<PIMInstruction> &instructions) {
    
    std::vector<PIMInstruction> simdInstructions;
    
    // Group instructions by type and opcode
    std::vector<PIMInstruction> progInstructions;
    std::vector<PIMInstruction> exeInstructions;
    std::vector<PIMInstruction> memReadInstructions;
    std::vector<PIMInstruction> memWriteInstructions;
    
    for (const auto &inst : instructions) {
        switch (inst.type) {
            case PIMInstructionType::PROG:
                progInstructions.push_back(inst);
                break;
            case PIMInstructionType::EXE:
                exeInstructions.push_back(inst);
                break;
            case PIMInstructionType::MEMORY_READ:
                memReadInstructions.push_back(inst);
                break;
            case PIMInstructionType::MEMORY_WRITE:
                memWriteInstructions.push_back(inst);
                break;
            default:
                // Pass through other instructions unchanged
                simdInstructions.push_back(inst);
                break;
        }
    }
    
    // Process PROG instructions
    if (!progInstructions.empty()) {
        // Group by opcode
        std::map<PIMOpcode, std::vector<PIMInstruction>> opcodeGroups;
        for (const auto &inst : progInstructions) {
            opcodeGroups[inst.opcode].push_back(inst);
        }
        
        // Generate SIMD LUT programming instructions for each opcode
        for (const auto &group : opcodeGroups) {
            auto simdProgs = generateSIMDLUTProgramming(group.first);
            simdInstructions.insert(simdInstructions.end(), simdProgs.begin(), simdProgs.end());
        }
    }
    
    // Process EXE instructions
    if (!exeInstructions.empty()) {
        // Group by opcode
        std::map<PIMOpcode, std::vector<PIMInstruction>> opcodeGroups;
        for (const auto &inst : exeInstructions) {
            opcodeGroups[inst.opcode].push_back(inst);
        }
        
        // Generate SIMD compute instructions for each opcode
        for (const auto &group : opcodeGroups) {
            auto simdExes = generateSIMDCompute(group.first);
            simdInstructions.insert(simdInstructions.end(), simdExes.begin(), simdExes.end());
        }
    }
    
    // Process memory read instructions
    if (!memReadInstructions.empty()) {
        std::vector<uint32_t> readAddresses;
        for (const auto &inst : memReadInstructions) {
            readAddresses.push_back(inst.address);
        }
        
        auto simdReads = generateSIMDMemoryAccess(true, readAddresses);
        simdInstructions.insert(simdInstructions.end(), simdReads.begin(), simdReads.end());
    }
    
    // Process memory write instructions
    if (!memWriteInstructions.empty()) {
        std::vector<uint32_t> writeAddresses;
        for (const auto &inst : memWriteInstructions) {
            writeAddresses.push_back(inst.address);
        }
        
        auto simdWrites = generateSIMDMemoryAccess(false, writeAddresses);
        simdInstructions.insert(simdInstructions.end(), simdWrites.begin(), simdWrites.end());
    }
    
    return simdInstructions;
}

std::vector<PIMInstruction> SIMDGenerator::generateSIMDLUTProgramming(PIMOpcode opcode) {
    std::vector<PIMInstruction> instructions;
    
    // Generate PROG instructions for each core in each cluster in the row
    for (uint32_t clusterId = 0; clusterId < ClustersPerRow; clusterId++) {
        for (uint32_t coreId = 0; coreId < CoresPerCluster; coreId++) {
            PIMInstruction progInst;
            progInst.type = PIMInstructionType::PROG;
            progInst.coreId = clusterId * CoresPerCluster + coreId;
            progInst.opcode = opcode;
            instructions.push_back(progInst);
        }
    }
    
    return instructions;
}

std::vector<PIMInstruction> SIMDGenerator::generateSIMDCompute(PIMOpcode opcode) {
    std::vector<PIMInstruction> instructions;
    
    // Generate EXE instruction
    PIMInstruction exeInst;
    exeInst.type = PIMInstructionType::EXE;
    exeInst.opcode = opcode;
    instructions.push_back(exeInst);
    
    // Generate END instruction
    PIMInstruction endInst;
    endInst.type = PIMInstructionType::END;
    instructions.push_back(endInst);
    
    return instructions;
}

std::vector<PIMInstruction> SIMDGenerator::generateSIMDMemoryAccess(
    bool isRead, const std::vector<uint32_t> &addresses) {
    
    std::vector<PIMInstruction> instructions;
    
    // Group addresses by row to minimize row activations
    std::map<uint32_t, std::vector<uint32_t>> rowGroups;
    for (uint32_t addr : addresses) {
        rowGroups[addr].push_back(addr);
    }
    
    // Generate memory access instructions for each row
    for (const auto &group : rowGroups) {
        PIMInstruction memInst;
        memInst.type = isRead ? PIMInstructionType::MEMORY_READ : PIMInstructionType::MEMORY_WRITE;
        memInst.address = group.first;
        instructions.push_back(memInst);
    }
    
    return instructions;
}

} // namespace ppim
