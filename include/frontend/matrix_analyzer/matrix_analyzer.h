#ifndef PPIM_MATRIX_ANALYZER_H
#define PPIM_MATRIX_ANALYZER_H

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include "frontend/parser/ast.h"

namespace ppim {

// Operation type for pPIM architecture
enum class PIMOperationType {
    MAC,        // Multiply and Accumulate
    RELU,       // ReLU activation function
    MAX_INDEX,  // Max index operation
    ADD,        // Addition operation
    MULTIPLY    // Multiplication operation
};

// Operation structure representing a decomposed matrix operation
struct PIMOperation {
    PIMOperationType type;
    std::string lhsMatrix;
    std::string rhsMatrix;
    std::string resultMatrix;
    int lhsRow;
    int lhsCol;
    int rhsRow;
    int rhsCol;
    int resultRow;
    int resultCol;
    
    PIMOperation() : type(PIMOperationType::MAC), lhsRow(0), lhsCol(0), rhsRow(0), rhsCol(0), resultRow(0), resultCol(0) {}
};

// Matrix analyzer class for decomposing matrix operations into pPIM-compatible operations
class MatrixAnalyzer {
public:
    MatrixAnalyzer();
    
    // Analyze a matrix multiplication expression
    bool analyzeMatrixMultiplication(const MatrixMultExprAST *expr);
    
    // Decompose a matrix multiplication into pPIM operations
    std::vector<PIMOperation> decomposeMatrixMultiplication(const MatrixMultExprAST *expr);
    
    // Get the number of pPIM clusters required for the operation
    int getRequiredClusters(const MatrixMultExprAST *expr);
    
    // Get the number of steps required for the operation
    int getRequiredSteps(const MatrixMultExprAST *expr);
    
    // Get the matrix dimensions
    std::pair<int, int> getMatrixDimensions(const std::string &name) const;
    
    // Set the matrix dimensions
    void setMatrixDimensions(const std::string &name, int rows, int cols);
    
    // Check if matrices can be multiplied
    bool canMultiply(const std::string &lhsMatrix, const std::string &rhsMatrix) const;
    
    // Get the dimensions of the result matrix
    std::pair<int, int> getResultDimensions(const std::string &lhsMatrix, const std::string &rhsMatrix) const;
    
private:
    // Map to store matrix dimensions (name -> {rows, cols})
    std::map<std::string, std::pair<int, int>> MatrixDimensions;
    
    // Helper function to decompose 8-bit MAC operation into 4-bit operations
    // as described in the reference paper (Fig. 6)
    std::vector<PIMOperation> decompose8BitMAC(const PIMOperation &macOp);
    
    // Helper function to map operations to pPIM clusters
    void mapOperationsToClusters(std::vector<PIMOperation> &operations, int numClusters);
};

} // namespace ppim

#endif // PPIM_MATRIX_ANALYZER_H
