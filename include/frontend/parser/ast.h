#ifndef PPIM_AST_H
#define PPIM_AST_H

#include <string>
#include <vector>
#include <memory>
#include "llvm/IR/Value.h"

namespace ppim {

// Base class for all expression nodes
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;
};

// Expression class for numeric literals
class NumberExprAST : public ExprAST {
    int Val;
public:
    NumberExprAST(int val) : Val(val) {}
    llvm::Value *codegen() override;
};

// Expression class for variable references
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &name) : Name(name) {}
    llvm::Value *codegen() override;
    const std::string &getName() const { return Name; }
};

// Expression class for matrix declarations
class MatrixDeclExprAST : public ExprAST {
    std::string Name;
    int Rows;
    int Cols;
    std::vector<int> Elements;
public:
    MatrixDeclExprAST(const std::string &name, int rows, int cols, 
                      std::vector<int> elements)
        : Name(name), Rows(rows), Cols(cols), Elements(std::move(elements)) {}
    llvm::Value *codegen() override;
};

// Expression class for matrix expressions
class MatrixExprAST : public ExprAST {
    std::string Name;
    int Rows;
    int Cols;
public:
    MatrixExprAST(const std::string &name, int rows, int cols)
        : Name(name), Rows(rows), Cols(cols) {}
    virtual llvm::Value *codegen() override;
    
    const std::string &getName() const { return Name; }
    int getRows() const { return Rows; }
    int getCols() const { return Cols; }
};

// Expression class for matrix multiplication
class MatrixMultExprAST : public ExprAST {
    std::unique_ptr<MatrixExprAST> LHS, RHS;
    std::string ResultName;
public:
    MatrixMultExprAST(std::unique_ptr<MatrixExprAST> lhs,
                      std::unique_ptr<MatrixExprAST> rhs,
                      const std::string &resultName)
        : LHS(std::move(lhs)), RHS(std::move(rhs)), ResultName(resultName) {}
    llvm::Value *codegen() override;
    
    const MatrixExprAST* getLHS() const { return LHS.get(); }
    const MatrixExprAST* getRHS() const { return RHS.get(); }
    const std::string &getResultName() const { return ResultName; }
};

// Expression class for a block of expressions
class BlockExprAST : public ExprAST {
    std::vector<std::unique_ptr<ExprAST>> Expressions;
public:
    BlockExprAST(std::vector<std::unique_ptr<ExprAST>> expressions)
        : Expressions(std::move(expressions)) {}
    llvm::Value *codegen() override;
};

} // namespace ppim

#endif // PPIM_AST_H
