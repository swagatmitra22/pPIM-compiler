#include "frontend/parser/ast.h"
#include <iostream>
#include "llvm/IR/Constants.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <map>

namespace ppim {

// Global variables for code generation
static llvm::LLVMContext *TheContext = nullptr;
static llvm::IRBuilder<> *Builder = nullptr;
static llvm::Module *TheModule = nullptr;
static std::map<std::string, llvm::Value*> NamedValues;
static std::map<std::string, std::pair<int, int>> MatrixDimensions; // Store matrix dimensions (rows, cols)

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, Val, true));
}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = NamedValues[Name];
    if (!V)
        std::cerr << "Unknown variable name: " << Name << std::endl;
    return V;
}

llvm::Value *MatrixDeclExprAST::codegen() {
    // Create a matrix type (represented as a pointer to array)
    llvm::Type *elementType = llvm::Type::getInt32Ty(*TheContext);
    llvm::ArrayType *matrixType = llvm::ArrayType::get(elementType, Rows * Cols);
    
    // Allocate memory for the matrix
    llvm::AllocaInst *matrixAlloc = Builder->CreateAlloca(matrixType, nullptr, Name);
    
    // Initialize matrix elements
    for (int i = 0; i < Rows * Cols; i++) {
        // Calculate indices
        int row = i / Cols;
        int col = i % Cols;
        
        // Create GEP for the element
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)),
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, i, true))
        };
        
        llvm::Value *elementPtr = Builder->CreateGEP(matrixType, matrixAlloc, indices, Name + "_elem_" + std::to_string(row) + "_" + std::to_string(col));
        
        // Store the value
        llvm::Value *val = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, Elements[i], true));
        Builder->CreateStore(val, elementPtr);
    }
    
    // Store the matrix in the symbol table
    NamedValues[Name] = matrixAlloc;
    
    // Store the matrix dimensions
    MatrixDimensions[Name] = std::make_pair(Rows, Cols);
    
    return matrixAlloc;
}

llvm::Value *MatrixExprAST::codegen() {
    llvm::Value *V = NamedValues[Name];
    if (!V)
        std::cerr << "Unknown matrix name: " << Name << std::endl;
    return V;
}

llvm::Value *MatrixMultExprAST::codegen() {
    // Get the matrices
    llvm::Value *lhsMatrix = LHS->codegen();
    llvm::Value *rhsMatrix = RHS->codegen();
    
    if (!lhsMatrix || !rhsMatrix)
        return nullptr;
    
    // Get matrix dimensions
    auto lhsDim = MatrixDimensions[LHS->getName()];
    auto rhsDim = MatrixDimensions[RHS->getName()];
    int lhsRows = lhsDim.first;
    int lhsCols = lhsDim.second;
    int rhsRows = rhsDim.first;
    int rhsCols = rhsDim.second;
    
    // Check if matrices can be multiplied
    if (lhsCols != rhsRows) {
        std::cerr << "Matrix dimensions do not match for multiplication: " 
                  << lhsRows << "x" << lhsCols << " * " 
                  << rhsRows << "x" << rhsCols << std::endl;
        return nullptr;
    }
    
    // Create result matrix type
    llvm::Type *elementType = llvm::Type::getInt32Ty(*TheContext);
    llvm::ArrayType *resultType = llvm::ArrayType::get(elementType, lhsRows * rhsCols);
    
    // Allocate memory for the result matrix
    llvm::AllocaInst *resultAlloc = Builder->CreateAlloca(resultType, nullptr, ResultName);
    
    // Initialize result matrix to zeros
    for (int i = 0; i < lhsRows * rhsCols; i++) {
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)),
            llvm::ConstantInt::get(*TheContext, llvm::APInt(32, i, true))
        };
        
        llvm::Value *elementPtr = Builder->CreateGEP(resultType, resultAlloc, indices, ResultName + "_elem_init_" + std::to_string(i));
        
        llvm::Value *zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
        Builder->CreateStore(zero, elementPtr);
    }
    
    // Implement matrix multiplication
    // This will be translated to pPIM instructions in the backend
    for (int i = 0; i < lhsRows; i++) {
        for (int j = 0; j < rhsCols; j++) {
            // Create accumulator for this element
            llvm::Value *sum = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
            
            for (int k = 0; k < lhsCols; k++) {
                // Get LHS element at [i][k]
                std::vector<llvm::Value*> lhsIndices = {
                    llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)),
                    llvm::ConstantInt::get(*TheContext, llvm::APInt(32, i * lhsCols + k, true))
                };
                llvm::Value *lhsElementPtr = Builder->CreateGEP(
                    llvm::ArrayType::get(elementType, lhsRows * lhsCols), 
                    lhsMatrix, 
                    lhsIndices, 
                    "lhs_elem_ptr");
                llvm::Value *lhsElement = Builder->CreateLoad(elementType, lhsElementPtr, "lhs_elem");
                
                // Get RHS element at [k][j]
                std::vector<llvm::Value*> rhsIndices = {
                    llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)),
                    llvm::ConstantInt::get(*TheContext, llvm::APInt(32, k * rhsCols + j, true))
                };
                llvm::Value *rhsElementPtr = Builder->CreateGEP(
                    llvm::ArrayType::get(elementType, rhsRows * rhsCols), 
                    rhsMatrix, 
                    rhsIndices, 
                    "rhs_elem_ptr");
                llvm::Value *rhsElement = Builder->CreateLoad(elementType, rhsElementPtr, "rhs_elem");
                
                // Multiply and accumulate
                llvm::Value *product = Builder->CreateMul(lhsElement, rhsElement, "product");
                sum = Builder->CreateAdd(sum, product, "sum");
            }
            
            // Store the result in the result matrix at [i][j]
            std::vector<llvm::Value*> resultIndices = {
                llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true)),
                llvm::ConstantInt::get(*TheContext, llvm::APInt(32, i * rhsCols + j, true))
            };
            llvm::Value *resultElementPtr = Builder->CreateGEP(resultType, resultAlloc, resultIndices, "result_elem_ptr");
            Builder->CreateStore(sum, resultElementPtr);
        }
    }
    
    // Store the result matrix in the symbol table
    NamedValues[ResultName] = resultAlloc;
    
    // Store the matrix dimensions
    MatrixDimensions[ResultName] = std::make_pair(lhsRows, rhsCols);
    
    return resultAlloc;
}

llvm::Value *BlockExprAST::codegen() {
    llvm::Value *lastVal = nullptr;
    for (auto &expr : Expressions) {
        lastVal = expr->codegen();
        if (!lastVal)
            return nullptr;
    }
    return lastVal;
}

// Helper function to set the code generation context
void setCodeGenContext(llvm::LLVMContext *context, llvm::IRBuilder<> *builder, llvm::Module *module) {
    TheContext = context;
    Builder = builder;
    TheModule = module;
}

} // namespace ppim
