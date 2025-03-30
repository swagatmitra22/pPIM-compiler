#include "frontend/ir_generator/ir_generator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include <iostream>

namespace ppim {

// External function to set the code generation context for AST nodes
extern void setCodeGenContext(llvm::LLVMContext *context, llvm::IRBuilder<> *builder, llvm::Module *module);

IRGenerator::IRGenerator(llvm::LLVMContext &context)
    : Context(context), Builder(context) {
    Module = std::make_unique<llvm::Module>("pPIM Module", Context);
}

bool IRGenerator::generateIR(std::unique_ptr<ExprAST> ast) {
    if (!ast) {
        std::cerr << "No AST to generate IR from" << std::endl;
        return false;
    }
    
    // Set up the code generation context for AST nodes
    setCodeGenContext();
    
    // Create a main function
    llvm::FunctionType *mainFuncType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(Context), false);
    llvm::Function *mainFunc = llvm::Function::Create(
        mainFuncType, llvm::Function::ExternalLinkage, "main", Module.get());
    
    // Create a basic block
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(Context, "entry", mainFunc);
    Builder.SetInsertPoint(BB);
    
    // Generate code from the AST
    llvm::Value *retVal = ast->codegen();
    if (!retVal) {
        std::cerr << "Code generation failed" << std::endl;
        mainFunc->eraseFromParent();
        return false;
    }
    
    // Create a return instruction
    Builder.CreateRetVoid();
    
    // Verify the generated code
    bool broken = llvm::verifyFunction(*mainFunc, &llvm::errs());
    if (broken) {
        std::cerr << "Function verification failed" << std::endl;
        mainFunc->eraseFromParent();
        return false;
    }
    
    return true;
}

std::unique_ptr<llvm::Module> IRGenerator::getModule() {
    return std::move(Module);
}

void IRGenerator::setCodeGenContext() {
    // Set the code generation context for AST nodes
    setCodeGenContext(&Context, &Builder, Module.get());
}

llvm::Function* IRGenerator::createMatrixMultFunction() {
    // Create a function type for matrix multiplication
    std::vector<llvm::Type*> paramTypes = {
        llvm::PointerType::get(llvm::Type::getInt32Ty(Context), 0), // Matrix A
        llvm::PointerType::get(llvm::Type::getInt32Ty(Context), 0), // Matrix B
        llvm::PointerType::get(llvm::Type::getInt32Ty(Context), 0), // Result Matrix
        llvm::Type::getInt32Ty(Context), // Rows A
        llvm::Type::getInt32Ty(Context), // Cols A / Rows B
        llvm::Type::getInt32Ty(Context)  // Cols B
    };
    
    llvm::FunctionType *funcType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(Context), paramTypes, false);
    
    // Create the function
    llvm::Function *func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, "matrix_mult", Module.get());
    
    // Set parameter names
    auto argIt = func->arg_begin();
    argIt->setName("A");
    ++argIt;
    argIt->setName("B");
    ++argIt;
    argIt->setName("C");
    ++argIt;
    argIt->setName("rowsA");
    ++argIt;
    argIt->setName("colsA");
    ++argIt;
    argIt->setName("colsB");
    
    // Create a basic block
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(Context, "entry", func);
    Builder.SetInsertPoint(BB);
    
    // Get function arguments
    llvm::Value *A = func->arg_begin();
    llvm::Value *B = func->arg_begin() + 1;
    llvm::Value *C = func->arg_begin() + 2;
    llvm::Value *rowsA = func->arg_begin() + 3;
    llvm::Value *colsA = func->arg_begin() + 4;
    llvm::Value *colsB = func->arg_begin() + 5;
    
    // Create loop variables
    llvm::AllocaInst *iAlloca = Builder.CreateAlloca(llvm::Type::getInt32Ty(Context), nullptr, "i");
    llvm::AllocaInst *jAlloca = Builder.CreateAlloca(llvm::Type::getInt32Ty(Context), nullptr, "j");
    llvm::AllocaInst *kAlloca = Builder.CreateAlloca(llvm::Type::getInt32Ty(Context), nullptr, "k");
    
    // Initialize i = 0
    Builder.CreateStore(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)), iAlloca);
    
    // Create basic blocks for the loops
    llvm::BasicBlock *outerLoopCond = llvm::BasicBlock::Create(Context, "outer_loop_cond", func);
    llvm::BasicBlock *outerLoopBody = llvm::BasicBlock::Create(Context, "outer_loop_body", func);
    llvm::BasicBlock *middleLoopCond = llvm::BasicBlock::Create(Context, "middle_loop_cond", func);
    llvm::BasicBlock *middleLoopBody = llvm::BasicBlock::Create(Context, "middle_loop_body", func);
    llvm::BasicBlock *innerLoopCond = llvm::BasicBlock::Create(Context, "inner_loop_cond", func);
    llvm::BasicBlock *innerLoopBody = llvm::BasicBlock::Create(Context, "inner_loop_body", func);
    llvm::BasicBlock *innerLoopInc = llvm::BasicBlock::Create(Context, "inner_loop_inc", func);
    llvm::BasicBlock *middleLoopInc = llvm::BasicBlock::Create(Context, "middle_loop_inc", func);
    llvm::BasicBlock *outerLoopInc = llvm::BasicBlock::Create(Context, "outer_loop_inc", func);
    llvm::BasicBlock *afterLoop = llvm::BasicBlock::Create(Context, "after_loop", func);
    
    // Jump to outer loop condition
    Builder.CreateBr(outerLoopCond);
    
    // Outer loop condition: i < rowsA
    Builder.SetInsertPoint(outerLoopCond);
    llvm::Value *i = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), iAlloca, "i");
    llvm::Value *outerCond = Builder.CreateICmpSLT(i, rowsA, "outer_cond");
    Builder.CreateCondBr(outerCond, outerLoopBody, afterLoop);
    
    // Outer loop body: initialize j = 0
    Builder.SetInsertPoint(outerLoopBody);
    Builder.CreateStore(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)), jAlloca);
    Builder.CreateBr(middleLoopCond);
    
    // Middle loop condition: j < colsB
    Builder.SetInsertPoint(middleLoopCond);
    llvm::Value *j = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), jAlloca, "j");
    llvm::Value *middleCond = Builder.CreateICmpSLT(j, colsB, "middle_cond");
    Builder.CreateCondBr(middleCond, middleLoopBody, outerLoopInc);
    
    // Middle loop body: initialize C[i][j] = 0, k = 0
    Builder.SetInsertPoint(middleLoopBody);
    // Calculate C[i][j] index
    llvm::Value *cIdx = Builder.CreateMul(i, colsB, "c_idx_row");
    cIdx = Builder.CreateAdd(cIdx, j, "c_idx");
    // Get pointer to C[i][j]
    llvm::Value *cPtr = Builder.CreateGEP(llvm::Type::getInt32Ty(Context), C, cIdx, "c_ptr");
    // Initialize C[i][j] = 0
    Builder.CreateStore(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)), cPtr);
    // Initialize k = 0
    Builder.CreateStore(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)), kAlloca);
    Builder.CreateBr(innerLoopCond);
    
    // Inner loop condition: k < colsA
    Builder.SetInsertPoint(innerLoopCond);
    llvm::Value *k = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), kAlloca, "k");
    llvm::Value *innerCond = Builder.CreateICmpSLT(k, colsA, "inner_cond");
    Builder.CreateCondBr(innerCond, innerLoopBody, middleLoopInc);
    
    // Inner loop body: C[i][j] += A[i][k] * B[k][j]
    Builder.SetInsertPoint(innerLoopBody);
    // Calculate A[i][k] index
    llvm::Value *aIdx = Builder.CreateMul(i, colsA, "a_idx_row");
    aIdx = Builder.CreateAdd(aIdx, k, "a_idx");
    // Get pointer to A[i][k]
    llvm::Value *aPtr = Builder.CreateGEP(llvm::Type::getInt32Ty(Context), A, aIdx, "a_ptr");
    // Load A[i][k]
    llvm::Value *aVal = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), aPtr, "a_val");
    
    // Calculate B[k][j] index
    llvm::Value *bIdx = Builder.CreateMul(k, colsB, "b_idx_row");
    bIdx = Builder.CreateAdd(bIdx, j, "b_idx");
    // Get pointer to B[k][j]
    llvm::Value *bPtr = Builder.CreateGEP(llvm::Type::getInt32Ty(Context), B, bIdx, "b_ptr");
    // Load B[k][j]
    llvm::Value *bVal = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), bPtr, "b_val");
    
    // Multiply A[i][k] * B[k][j]
    llvm::Value *prod = Builder.CreateMul(aVal, bVal, "prod");
    
    // Load current C[i][j]
    llvm::Value *cVal = Builder.CreateLoad(llvm::Type::getInt32Ty(Context), cPtr, "c_val");
    
    // Add product to C[i][j]
    llvm::Value *sum = Builder.CreateAdd(cVal, prod, "sum");
    
    // Store result back to C[i][j]
    Builder.CreateStore(sum, cPtr);
    
    // Jump to inner loop increment
    Builder.CreateBr(innerLoopInc);
    
    // Inner loop increment: k++
    Builder.SetInsertPoint(innerLoopInc);
    llvm::Value *kInc = Builder.CreateAdd(k, llvm::ConstantInt::get(Context, llvm::APInt(32, 1, true)), "k_inc");
    Builder.CreateStore(kInc, kAlloca);
    Builder.CreateBr(innerLoopCond);
    
    // Middle loop increment: j++
    Builder.SetInsertPoint(middleLoopInc);
    llvm::Value *jInc = Builder.CreateAdd(j, llvm::ConstantInt::get(Context, llvm::APInt(32, 1, true)), "j_inc");
    Builder.CreateStore(jInc, jAlloca);
    Builder.CreateBr(middleLoopCond);
    
    // Outer loop increment: i++
    Builder.SetInsertPoint(outerLoopInc);
    llvm::Value *iInc = Builder.CreateAdd(i, llvm::ConstantInt::get(Context, llvm::APInt(32, 1, true)), "i_inc");
    Builder.CreateStore(iInc, iAlloca);
    Builder.CreateBr(outerLoopCond);
    
    // After loop: return
    Builder.SetInsertPoint(afterLoop);
    Builder.CreateRetVoid();
    
    return func;
}

bool IRGenerator::generateMatrixMultCode(const MatrixMultExprAST* multExpr) {
    if (!multExpr) {
        std::cerr << "Invalid matrix multiplication expression" << std::endl;
        return false;
    }
    
    // Get matrix dimensions
    auto lhsDim = getMatrixDimensions(multExpr->getLHS()->getName());
    auto rhsDim = getMatrixDimensions(multExpr->getRHS()->getName());
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
    
    // Create a function for matrix multiplication if it doesn't exist
    llvm::Function *matMultFunc = Module->getFunction("matrix_mult");
    if (!matMultFunc) {
        matMultFunc = createMatrixMultFunction();
    }
    
    // Get the matrices
    llvm::Value *lhsMatrix = NamedValues[multExpr->getLHS()->getName()];
    llvm::Value *rhsMatrix = NamedValues[multExpr->getRHS()->getName()];
    
    // Create result matrix
    llvm::Type *elementType = llvm::Type::getInt32Ty(Context);
    llvm::ArrayType *resultType = llvm::ArrayType::get(elementType, lhsRows * rhsCols);
    llvm::AllocaInst *resultAlloc = Builder.CreateAlloca(resultType, nullptr, multExpr->getResultName());
    
    // Store the result matrix in the symbol table
    NamedValues[multExpr->getResultName()] = resultAlloc;
    
    // Store the matrix dimensions
    setMatrixDimensions(multExpr->getResultName(), lhsRows, rhsCols);
    
    // Create arguments for the matrix multiplication function
    std::vector<llvm::Value*> args = {
        lhsMatrix,
        rhsMatrix,
        resultAlloc,
        llvm::ConstantInt::get(Context, llvm::APInt(32, lhsRows, true)),
        llvm::ConstantInt::get(Context, llvm::APInt(32, lhsCols, true)),
        llvm::ConstantInt::get(Context, llvm::APInt(32, rhsCols, true))
    };
    
    // Call the matrix multiplication function
    Builder.CreateCall(matMultFunc, args);
    
    return true;
}

std::pair<int, int> IRGenerator::getMatrixDimensions(const std::string &name) {
    auto it = MatrixDimensions.find(name);
    if (it != MatrixDimensions.end()) {
        return it->second;
    }
    
    // Default dimensions if not found
    return std::make_pair(0, 0);
}

void IRGenerator::setMatrixDimensions(const std::string &name, int rows, int cols) {
    MatrixDimensions[name] = std::make_pair(rows, cols);
}

} // namespace ppim
