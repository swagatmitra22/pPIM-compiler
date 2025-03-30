#ifndef PPIM_MEMORY_MAPPER_H
#define PPIM_MEMORY_MAPPER_H

#include <vector>
#include <map>
#include <string>
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

namespace ppim {

// Structure to represent a physical memory location in the pPIM architecture
struct PhysicalMemoryLocation {
    uint32_t bankId;       // Bank ID
    uint32_t subarrayId;   // Subarray ID within the bank
    uint32_t rowAddress;   // Row address within the subarray (9-bit as per Section IV-D)
    uint32_t columnOffset; // Column offset within the row
    
    PhysicalMemoryLocation() 
        : bankId(0), subarrayId(0), rowAddress(0), columnOffset(0) {}
    
    PhysicalMemoryLocation(uint32_t bank, uint32_t subarray, uint32_t row, uint32_t col) 
        : bankId(bank), subarrayId(subarray), rowAddress(row), columnOffset(col) {}
};

// Structure to represent a matrix in physical memory
struct MatrixMemoryLayout {
    PhysicalMemoryLocation startLocation;
    uint32_t rows;
    uint32_t cols;
    bool rowMajor;  // True if matrix is stored in row-major format
    
    MatrixMemoryLayout() 
        : rows(0), cols(0), rowMajor(true) {}
    
    MatrixMemoryLayout(PhysicalMemoryLocation start, uint32_t r, uint32_t c, bool isRowMajor) 
        : startLocation(start), rows(r), cols(c), rowMajor(isRowMajor) {}
};

// Memory mapper class
class MemoryMapper {
public:
    MemoryMapper();
    
    // Initialize the memory mapper with architecture parameters
    void initialize(uint32_t numBanks, uint32_t numSubarraysPerBank, 
                   uint32_t numRowsPerSubarray, uint32_t numColsPerRow);
    
    // Map a matrix to physical memory
    MatrixMemoryLayout mapMatrix(const std::string &name, uint32_t rows, uint32_t cols);
    
    // Get the physical memory location for a matrix element
    PhysicalMemoryLocation getElementLocation(const MatrixMemoryLayout &matrix, uint32_t row, uint32_t col);
    
    // Map LLVM values to physical memory locations
    bool mapValues(llvm::Module *module);
    
    // Get the physical memory location for an LLVM value
    PhysicalMemoryLocation getValueLocation(llvm::Value *value);
    
    // Get the matrix memory layout for a matrix name
    MatrixMemoryLayout getMatrixLayout(const std::string &name);
    
    // Check if a matrix is already mapped
    bool isMatrixMapped(const std::string &name);
    
    // Optimize memory layout for matrix multiplication
    bool optimizeForMatrixMultiplication(const std::string &matrixA, const std::string &matrixB, 
                                        const std::string &resultMatrix);
    
    // Get the pPIM cluster ID for a physical memory location
    uint32_t getClusterIdForLocation(const PhysicalMemoryLocation &location);
    
private:
    // Architecture parameters
    uint32_t NumBanks;
    uint32_t NumSubarraysPerBank;
    uint32_t NumRowsPerSubarray;
    uint32_t NumColsPerRow;
    uint32_t NumClustersPerSubarray;  // Typically 4 as per the reference paper
    
    // Memory allocation tracking
    uint32_t NextAvailableBank;
    uint32_t NextAvailableSubarray;
    uint32_t NextAvailableRow;
    uint32_t NextAvailableCol;
    
    // Maps to track memory allocations
    std::map<std::string, MatrixMemoryLayout> MatrixLayouts;
    std::map<llvm::Value*, PhysicalMemoryLocation> ValueLocations;
    
    // Helper function to allocate memory for a matrix
    PhysicalMemoryLocation allocateMemory(uint32_t size);
    
    // Helper function to check if a memory region is available
    bool isMemoryAvailable(const PhysicalMemoryLocation &location, uint32_t size);
};

} // namespace ppim

#endif // PPIM_MEMORY_MAPPER_H
