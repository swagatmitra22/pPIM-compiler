#include "backend/memory_mapper/memory_mapper.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ppim {

MemoryMapper::MemoryMapper()
    : NumBanks(8), NumSubarraysPerBank(16), NumRowsPerSubarray(512), NumColsPerRow(2048),
      NumClustersPerSubarray(4), NextAvailableBank(0), NextAvailableSubarray(0),
      NextAvailableRow(0), NextAvailableCol(0) {
    // Default initialization with typical pPIM architecture parameters
    // 8 banks, 16 subarrays per bank, 512 rows per subarray, 2048 columns per row
    // 4 clusters per subarray as described in the reference paper
}

void MemoryMapper::initialize(uint32_t numBanks, uint32_t numSubarraysPerBank, 
                             uint32_t numRowsPerSubarray, uint32_t numColsPerRow) {
    NumBanks = numBanks;
    NumSubarraysPerBank = numSubarraysPerBank;
    NumRowsPerSubarray = numRowsPerSubarray;
    NumColsPerRow = numColsPerRow;
    
    // Reset allocation tracking
    NextAvailableBank = 0;
    NextAvailableSubarray = 0;
    NextAvailableRow = 0;
    NextAvailableCol = 0;
    
    // Clear maps
    MatrixLayouts.clear();
    ValueLocations.clear();
}

MatrixMemoryLayout MemoryMapper::mapMatrix(const std::string &name, uint32_t rows, uint32_t cols) {
    // Check if matrix is already mapped
    if (isMatrixMapped(name)) {
        return MatrixLayouts[name];
    }
    
    // Calculate total size in bytes (assuming 1 byte per element for integer operands)
    uint32_t totalSize = rows * cols;
    
    // Allocate memory for the matrix
    PhysicalMemoryLocation startLocation = allocateMemory(totalSize);
    
    // Create matrix memory layout
    MatrixMemoryLayout layout(startLocation, rows, cols, true); // Use row-major format by default
    
    // Store the layout
    MatrixLayouts[name] = layout;
    
    return layout;
}

PhysicalMemoryLocation MemoryMapper::getElementLocation(const MatrixMemoryLayout &matrix, uint32_t row, uint32_t col) {
    if (row >= matrix.rows || col >= matrix.cols) {
        std::cerr << "Error: Matrix index out of bounds" << std::endl;
        return PhysicalMemoryLocation();
    }
    
    // Calculate offset based on storage format (row-major or column-major)
    uint32_t offset;
    if (matrix.rowMajor) {
        offset = row * matrix.cols + col;
    } else {
        offset = col * matrix.rows + row;
    }
    
    // Calculate physical location
    PhysicalMemoryLocation location = matrix.startLocation;
    
    // Add offset to column
    location.columnOffset += offset;
    
    // Handle column overflow
    while (location.columnOffset >= NumColsPerRow) {
        location.columnOffset -= NumColsPerRow;
        location.rowAddress++;
        
        // Handle row overflow
        if (location.rowAddress >= NumRowsPerSubarray) {
            location.rowAddress = 0;
            location.subarrayId++;
            
            // Handle subarray overflow
            if (location.subarrayId >= NumSubarraysPerBank) {
                location.subarrayId = 0;
                location.bankId++;
                
                // Handle bank overflow (wrap around if needed)
                if (location.bankId >= NumBanks) {
                    location.bankId = 0;
                }
            }
        }
    }
    
    return location;
}

bool MemoryMapper::mapValues(llvm::Module *module) {
    if (!module) {
        std::cerr << "Invalid module" << std::endl;
        return false;
    }
    
    // Clear existing mappings
    ValueLocations.clear();
    
    // Iterate through global variables
    for (auto &G : module->globals()) {
        // Skip external globals
        if (G.isDeclaration()) {
            continue;
        }
        
        // Allocate memory for the global variable
        uint32_t size = 0;
        llvm::Type *type = G.getValueType();
        
        // Calculate size based on type
        if (type->isArrayTy()) {
            size = type->getArrayNumElements();
            llvm::Type *elemType = type->getArrayElementType();
            if (elemType->isIntegerTy()) {
                size *= (elemType->getIntegerBitWidth() + 7) / 8; // Round up to bytes
            }
        } else if (type->isIntegerTy()) {
            size = (type->getIntegerBitWidth() + 7) / 8; // Round up to bytes
        }
        
        // Allocate memory
        PhysicalMemoryLocation location = allocateMemory(size);
        
        // Store the mapping
        ValueLocations[&G] = location;
    }
    
    // Iterate through functions
    for (auto &F : *module) {
        // Skip declarations
        if (F.isDeclaration()) {
            continue;
        }
        
        // Iterate through basic blocks
        for (auto &BB : F) {
            // Iterate through instructions
            for (auto &I : BB) {
                // Handle allocas
                if (llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                    // Calculate size
                    uint32_t size = 0;
                    llvm::Type *type = alloca->getAllocatedType();
                    
                    // Calculate size based on type
                    if (type->isArrayTy()) {
                        size = type->getArrayNumElements();
                        llvm::Type *elemType = type->getArrayElementType();
                        if (elemType->isIntegerTy()) {
                            size *= (elemType->getIntegerBitWidth() + 7) / 8; // Round up to bytes
                        }
                    } else if (type->isIntegerTy()) {
                        size = (type->getIntegerBitWidth() + 7) / 8; // Round up to bytes
                    }
                    
                    // Allocate memory
                    PhysicalMemoryLocation location = allocateMemory(size);
                    
                    // Store the mapping
                    ValueLocations[alloca] = location;
                }
            }
        }
    }
    
    return true;
}

PhysicalMemoryLocation MemoryMapper::getValueLocation(llvm::Value *value) {
    auto it = ValueLocations.find(value);
    if (it != ValueLocations.end()) {
        return it->second;
    }
    
    // Return default location if not found
    return PhysicalMemoryLocation();
}

MatrixMemoryLayout MemoryMapper::getMatrixLayout(const std::string &name) {
    auto it = MatrixLayouts.find(name);
    if (it != MatrixLayouts.end()) {
        return it->second;
    }
    
    // Return default layout if not found
    return MatrixMemoryLayout();
}

bool MemoryMapper::isMatrixMapped(const std::string &name) {
    return MatrixLayouts.find(name) != MatrixLayouts.end();
}

bool MemoryMapper::optimizeForMatrixMultiplication(const std::string &matrixA, const std::string &matrixB, 
                                                  const std::string &resultMatrix) {
    // Check if all matrices are mapped
    if (!isMatrixMapped(matrixA) || !isMatrixMapped(matrixB) || !isMatrixMapped(resultMatrix)) {
        std::cerr << "Error: One or more matrices not mapped" << std::endl;
        return false;
    }
    
    // Get matrix layouts
    MatrixMemoryLayout layoutA = MatrixLayouts[matrixA];
    MatrixMemoryLayout layoutB = MatrixLayouts[matrixB];
    MatrixMemoryLayout layoutC = MatrixLayouts[resultMatrix];
    
    // Check if matrices can be multiplied
    if (layoutA.cols != layoutB.rows) {
        std::cerr << "Error: Matrix dimensions do not match for multiplication" << std::endl;
        return false;
    }
    
    // Check if result matrix has correct dimensions
    if (layoutC.rows != layoutA.rows || layoutC.cols != layoutB.cols) {
        std::cerr << "Error: Result matrix has incorrect dimensions" << std::endl;
        return false;
    }
    
    // Optimize memory layout for matrix multiplication
    // In a real implementation, this would involve:
    // 1. Reorganizing matrices to minimize data movement
    // 2. Placing matrices in subarrays that are close to the pPIM clusters
    // 3. Aligning matrices to optimize access patterns
    
    // For now, we'll just return true as a placeholder
    return true;
}

uint32_t MemoryMapper::getClusterIdForLocation(const PhysicalMemoryLocation &location) {
    // Calculate cluster ID based on physical location
    // In the pPIM architecture, each subarray has 4 clusters
    // The cluster ID is determined by the subarray ID and the row address
    
    // Calculate global subarray ID
    uint32_t globalSubarrayId = location.bankId * NumSubarraysPerBank + location.subarrayId;
    
    // Calculate cluster ID within the subarray (based on row address)
    // Each cluster is responsible for a range of rows within the subarray
    uint32_t rowsPerCluster = NumRowsPerSubarray / NumClustersPerSubarray;
    uint32_t clusterIdWithinSubarray = location.rowAddress / rowsPerCluster;
    
    // Calculate global cluster ID
    uint32_t globalClusterId = globalSubarrayId * NumClustersPerSubarray + clusterIdWithinSubarray;
    
    return globalClusterId;
}

PhysicalMemoryLocation MemoryMapper::allocateMemory(uint32_t size) {
    // Create a new physical memory location
    PhysicalMemoryLocation location(NextAvailableBank, NextAvailableSubarray, NextAvailableRow, NextAvailableCol);
    
    // Update next available location
    NextAvailableCol += size;
    
    // Handle column overflow
    while (NextAvailableCol >= NumColsPerRow) {
        NextAvailableCol -= NumColsPerRow;
        NextAvailableRow++;
        
        // Handle row overflow
        if (NextAvailableRow >= NumRowsPerSubarray) {
            NextAvailableRow = 0;
            NextAvailableSubarray++;
            
            // Handle subarray overflow
            if (NextAvailableSubarray >= NumSubarraysPerBank) {
                NextAvailableSubarray = 0;
                NextAvailableBank++;
                
                // Handle bank overflow (wrap around if needed)
                if (NextAvailableBank >= NumBanks) {
                    NextAvailableBank = 0;
                }
            }
        }
    }
    
    return location;
}

bool MemoryMapper::isMemoryAvailable(const PhysicalMemoryLocation &location, uint32_t size) {
    // Check if the memory region is available
    // In a real implementation, this would check for conflicts with existing allocations
    
    // For now, we'll just return true as a placeholder
    return true;
}

} // namespace ppim
