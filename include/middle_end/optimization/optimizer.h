#ifndef PPIM_OPTIMIZER_H
#define PPIM_OPTIMIZER_H

#include <memory>
#include <vector>
#include <string>
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace ppim {

// Optimization pass for matrix multiplication operations
class MatrixMultOptimizationPass : public llvm::PassInfoMixin<MatrixMultOptimizationPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    
private:
    // Identify matrix multiplication patterns
    bool identifyMatrixMultPattern(llvm::Function &F);
    
    // Apply loop tiling optimization
    bool applyLoopTiling(llvm::Loop *L, unsigned TileSize);
    
    // Apply loop unrolling optimization
    bool applyLoopUnrolling(llvm::Loop *L, unsigned UnrollFactor);
};

// Optimization pass for memory access patterns
class MemoryAccessOptimizationPass : public llvm::PassInfoMixin<MemoryAccessOptimizationPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
    
private:
    // Optimize memory access patterns for pPIM architecture
    bool optimizeMemoryAccess(llvm::Function &F);
    
    // Map memory accesses to pPIM clusters
    bool mapMemoryToClusters(llvm::Function &F);
};

// Main optimizer class
class Optimizer {
public:
    Optimizer();
    
    // Apply optimizations to the IR module
    bool optimizeIR(llvm::Module *module);
    
    // Set tiling size for matrix multiplication
    void setTilingSize(unsigned size) { TilingSize = size; }
    
    // Set unrolling factor for loops
    void setUnrollingFactor(unsigned factor) { UnrollingFactor = factor; }
    
    // Set the number of pPIM clusters to target
    void setNumClusters(unsigned num) { NumClusters = num; }
    
private:
    // Optimization parameters
    unsigned TilingSize;
    unsigned UnrollingFactor;
    unsigned NumClusters;
    
    // Initialize optimization passes
    void initializePasses();
    
    // Apply matrix multiplication optimizations
    bool optimizeMatrixMult(llvm::Module *module);
    
    // Apply memory access optimizations
    bool optimizeMemoryAccess(llvm::Module *module);
    
    // Apply SIMD vectorization
    bool applySIMDVectorization(llvm::Module *module);
    
    // Map operations to pPIM clusters
    bool mapOperationsToClusters(llvm::Module *module);
};

} // namespace ppim

#endif // PPIM_OPTIMIZER_H
