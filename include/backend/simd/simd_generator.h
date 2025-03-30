#ifndef PPIM_SIMD_GENERATOR_H
#define PPIM_SIMD_GENERATOR_H

#include <vector>
#include "backend/code_generator/code_generator.h"
#include "backend/memory_mapper/memory_mapper.h"

namespace ppim {

// SIMD instruction generator class
class SIMDGenerator {
public:
    SIMDGenerator();
    
    // Initialize the SIMD generator with architecture parameters
    void initialize(uint32_t clustersPerRow, uint32_t coresPerCluster);
    
    // Generate SIMD instructions for matrix multiplication
    std::vector<PIMInstruction> generateMatrixMultSIMD(const std::string &matrixA, 
                                                     const std::string &matrixB,
                                                     const std::string &resultMatrix,
                                                     const MemoryMapper &memMapper);
    
    // Generate atomic instructions for a stream of identical operations
    std::vector<PIMInstruction> generateAtomicInstructions(PIMOpcode opcode, 
                                                         uint32_t numOperations);
    
    // Map matrix operations to SIMD instructions
    std::vector<PIMInstruction> mapToSIMD(const std::vector<PIMInstruction> &instructions);
    
private:
    uint32_t ClustersPerRow;    // Number of clusters in a row (typically 4)
    uint32_t CoresPerCluster;   // Number of cores per cluster (typically 9)
    
    // Generate SIMD LUT programming instructions
    std::vector<PIMInstruction> generateSIMDLUTProgramming(PIMOpcode opcode);
    
    // Generate SIMD compute instructions
    std::vector<PIMInstruction> generateSIMDCompute(PIMOpcode opcode);
    
    // Generate SIMD memory access instructions
    std::vector<PIMInstruction> generateSIMDMemoryAccess(bool isRead, 
                                                       const std::vector<uint32_t> &addresses);
};

} // namespace ppim

#endif // PPIM_SIMD_GENERATOR_H
