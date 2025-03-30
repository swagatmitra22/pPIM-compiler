#ifndef PPIM_IR_GENERATOR_H
#define PPIM_IR_GENERATOR_H

#include <memory>
#include <string>
#include <iostream>
#include <map>
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "frontend/parser/ast.h"

namespace ppim {

// IR generator class for converting AST to LLVM IR
class IRGenerator {
public:
    IRGenerator(llvm::LLVMContext &context);
    
    // Generate LLVM IR from the AST
    bool generateIR(std::unique_ptr<ExprAST> ast);
    
    // Get the generated module
    std::unique_ptr<llvm::Module> getModule();
    
    // Set the code generation context for AST nodes
    void setCodeGenContext();

private:
    llvm::LLVMContext &Context;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> Module;
    
    // Symbol table for named values
    std::map<std::string, llvm::Value*> NamedValues;
    
    // Matrix dimensions table (name -> {rows, cols})
    std::map<std::string, std::pair<int, int>> MatrixDimensions;
    
    // Create a function for matrix multiplication
    llvm::Function* createMatrixMultFunction();
    
    // Generate matrix multiplication code
    bool generateMatrixMultCode(const MatrixMultExprAST* multExpr);
    
    // Helper function to get matrix dimensions
    std::pair<int, int> getMatrixDimensions(const std::string &name);
    
    // Helper function to set matrix dimensions
    void setMatrixDimensions(const std::string &name, int rows, int cols);
};

} // namespace ppim

#endif // PPIM_IR_GENERATOR_H
