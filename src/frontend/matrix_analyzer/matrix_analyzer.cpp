#include "frontend/matrix_analyzer/matrix_analyzer.h"
#include <iostream>
#include <algorithm>

namespace ppim {

MatrixAnalyzer::MatrixAnalyzer() {}

bool MatrixAnalyzer::analyzeMatrixMultiplication(const MatrixMultExprAST *expr) {
    if (!expr) {
        std::cerr << "Invalid matrix multiplication expression" << std::endl;
        return false;
    }
    
    // Get matrix dimensions
    auto lhsDim = getMatrixDimensions(expr->getLHS()->getName());
    auto rhsDim = getMatrixDimensions(expr->getRHS()->getName());
    int lhsRows = lhsDim.first;
    int lhsCols = lhsDim.second;
    int rhsRows = rhsDim.first;
    int rhsCols = rhsDim.second;
    
    // Check if matrices can be multiplied
    if (lhsCols != rhsRows) {
        std::cerr << "Matrix dimensions do not match for multiplication: "
                  << lhsRows << "x" << lhsCols << " * "
                  << rhsRows << "x" << rhsCols << std::endl;
        return false;
    }
    
    // Analyze the operation for pPIM mapping
    // This involves:
    // 1. Breaking down the matrix multiplication into smaller operations
    // 2. Mapping these operations to the pPIM's LUT-based computing model
    // 3. Determining resource requirements
    
    return true;
}

std::vector<PIMOperation> MatrixAnalyzer::decomposeMatrixMultiplication(const MatrixMultExprAST *expr) {
    std::vector<PIMOperation> operations;
    
    if (!expr) {
        std::cerr << "Invalid matrix multiplication expression" << std::endl;
        return operations;
    }
    
    // Get matrix dimensions
    auto lhsDim = getMatrixDimensions(expr->getLHS()->getName());
    auto rhsDim = getMatrixDimensions(expr->getRHS()->getName());
    int lhsRows = lhsDim.first;
    int lhsCols = lhsDim.second;
    int rhsRows = rhsDim.first;
    int rhsCols = rhsDim.second;
    
    // Check if matrices can be multiplied
    if (lhsCols != rhsRows) {
        std::cerr << "Matrix dimensions do not match for multiplication: "
                  << lhsRows << "x" << lhsCols << " * "
                  << rhsRows << "x" << rhsCols << std::endl;
        return operations;
    }
    
    // Decompose the matrix multiplication into a sequence of MAC operations
    // Each MAC operation will be further decomposed into pPIM instructions
    
    for (int i = 0; i < lhsRows; i++) {
        for (int j = 0; j < rhsCols; j++) {
            for (int k = 0; k < lhsCols; k++) {
                // Create a MAC operation for C[i][j] += A[i][k] * B[k][j]
                PIMOperation op;
                op.type = PIMOperationType::MAC;
                op.lhsMatrix = expr->getLHS()->getName();
                op.rhsMatrix = expr->getRHS()->getName();
                op.resultMatrix = expr->getResultName();
                op.lhsRow = i;
                op.lhsCol = k;
                op.rhsRow = k;
                op.rhsCol = j;
                op.resultRow = i;
                op.resultCol = j;
                
                operations.push_back(op);
                
                // For 8-bit operations, decompose into 4-bit operations as per Fig. 6
                if (operations.back().type == PIMOperationType::MAC) {
                    std::vector<PIMOperation> decomposedOps = decompose8BitMAC(operations.back());
                    operations.pop_back();
                    operations.insert(operations.end(), decomposedOps.begin(), decomposedOps.end());
                }
            }
        }
    }
    
    // Map operations to pPIM clusters
    int numClusters = getRequiredClusters(expr);
    mapOperationsToClusters(operations, numClusters);
    
    return operations;
}

int MatrixAnalyzer::getRequiredClusters(const MatrixMultExprAST *expr) {
    if (!expr) {
        return 0;
    }
    
    // Get matrix dimensions
    auto lhsDim = getMatrixDimensions(expr->getLHS()->getName());
    auto rhsDim = getMatrixDimensions(expr->getRHS()->getName());
    int lhsRows = lhsDim.first;
    int rhsCols = rhsDim.second;
    
    // Each result element requires one cluster
    return lhsRows * rhsCols;
}

int MatrixAnalyzer::getRequiredSteps(const MatrixMultExprAST *expr) {
    if (!expr) {
        return 0;
    }
    
    // Get matrix dimensions
    auto lhsDim = getMatrixDimensions(expr->getLHS()->getName());
    auto rhsDim = getMatrixDimensions(expr->getRHS()->getName());
    int lhsCols = lhsDim.second;
    
    // Each result element requires lhsCols MAC operations
    // Each MAC operation requires 8 steps as per Fig. 6
    return lhsCols * 8;
}

std::pair<int, int> MatrixAnalyzer::getMatrixDimensions(const std::string &name) const {
    auto it = MatrixDimensions.find(name);
    if (it != MatrixDimensions.end()) {
        return it->second;
    }
    
    // Default dimensions if not found
    return std::make_pair(0, 0);
}

void MatrixAnalyzer::setMatrixDimensions(const std::string &name, int rows, int cols) {
    MatrixDimensions[name] = std::make_pair(rows, cols);
}

bool MatrixAnalyzer::canMultiply(const std::string &lhsMatrix, const std::string &rhsMatrix) const {
    auto lhsDim = getMatrixDimensions(lhsMatrix);
    auto rhsDim = getMatrixDimensions(rhsMatrix);
    
    return lhsDim.second == rhsDim.first;
}

std::pair<int, int> MatrixAnalyzer::getResultDimensions(const std::string &lhsMatrix, const std::string &rhsMatrix) const {
    auto lhsDim = getMatrixDimensions(lhsMatrix);
    auto rhsDim = getMatrixDimensions(rhsMatrix);
    
    if (lhsDim.second != rhsDim.first) {
        // Cannot multiply
        return std::make_pair(0, 0);
    }
    
    return std::make_pair(lhsDim.first, rhsDim.second);
}

std::vector<PIMOperation> MatrixAnalyzer::decompose8BitMAC(const PIMOperation &macOp) {
    std::vector<PIMOperation> operations;
    
    // Based on Fig. 6 in the reference paper, an 8-bit MAC operation
    // is decomposed into 8 stages of 4-bit operations
    
    // Stage 1: Split inputs into 4-bit segments and perform partial multiplications
    PIMOperation stage1;
    stage1.type = PIMOperationType::MULTIPLY;
    stage1.lhsMatrix = macOp.lhsMatrix + "_L"; // Lower 4 bits of lhs
    stage1.rhsMatrix = macOp.rhsMatrix + "_L"; // Lower 4 bits of rhs
    stage1.resultMatrix = "V0"; // Partial product
    operations.push_back(stage1);
    
    PIMOperation stage2;
    stage2.type = PIMOperationType::MULTIPLY;
    stage2.lhsMatrix = macOp.lhsMatrix + "_L"; // Lower 4 bits of lhs
    stage2.rhsMatrix = macOp.rhsMatrix + "_H"; // Higher 4 bits of rhs
    stage2.resultMatrix = "V1"; // Partial product
    operations.push_back(stage2);
    
    PIMOperation stage3;
    stage3.type = PIMOperationType::MULTIPLY;
    stage3.lhsMatrix = macOp.lhsMatrix + "_H"; // Higher 4 bits of lhs
    stage3.rhsMatrix = macOp.rhsMatrix + "_L"; // Lower 4 bits of rhs
    stage3.resultMatrix = "V2"; // Partial product
    operations.push_back(stage3);
    
    PIMOperation stage4;
    stage4.type = PIMOperationType::MULTIPLY;
    stage4.lhsMatrix = macOp.lhsMatrix + "_H"; // Higher 4 bits of lhs
    stage4.rhsMatrix = macOp.rhsMatrix + "_H"; // Higher 4 bits of rhs
    stage4.resultMatrix = "V3"; // Partial product
    operations.push_back(stage4);
    
    // Stages 5-8: Accumulate partial products
    // These stages correspond to the addition operations in Fig. 6
    
    PIMOperation stage5;
    stage5.type = PIMOperationType::ADD;
    stage5.lhsMatrix = "V0_H"; // Higher 4 bits of V0
    stage5.rhsMatrix = "V1_L"; // Lower 4 bits of V1
    stage5.resultMatrix = "A1"; // Accumulator segment 1
    operations.push_back(stage5);
    
    PIMOperation stage6;
    stage6.type = PIMOperationType::ADD;
    stage6.lhsMatrix = "A1";
    stage6.rhsMatrix = "V2_L";
    stage6.resultMatrix = "A2"; // Accumulator segment 2
    operations.push_back(stage6);
    
    PIMOperation stage7;
    stage7.type = PIMOperationType::ADD;
    stage7.lhsMatrix = "V1_H";
    stage7.rhsMatrix = "V2_H";
    stage7.resultMatrix = "A3"; // Accumulator segment 3
    operations.push_back(stage7);
    
    PIMOperation stage8;
    stage8.type = PIMOperationType::ADD;
    stage8.lhsMatrix = "A3";
    stage8.rhsMatrix = "V3";
    stage8.resultMatrix = macOp.resultMatrix; // Final result
    operations.push_back(stage8);
    
    return operations;
}

void MatrixAnalyzer::mapOperationsToClusters(std::vector<PIMOperation> &operations, int numClusters) {
    // In a real implementation, this would map operations to specific pPIM clusters
    // based on the physical layout of the pPIM architecture
    
    // For now, we'll just distribute operations evenly across clusters
    for (size_t i = 0; i < operations.size(); i++) {
        // Assign each operation to a cluster
        int clusterId = i % numClusters;
        
        // In a real implementation, we would set additional fields in the PIMOperation
        // to indicate which cluster should execute it
    }
}

} // namespace ppim
