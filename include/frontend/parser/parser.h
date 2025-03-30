#ifndef PPIM_PARSER_H
#define PPIM_PARSER_H

#include <string>
#include <memory>
#include <vector>
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace ppim {

// Forward declarations
class ExprAST;
class MatrixExprAST;

// Parser class for matrix multiplication code
class Parser {
public:
    Parser(llvm::LLVMContext &context);
    ~Parser();

    // Parse the input file and generate AST
    std::unique_ptr<ExprAST> parseFile(const std::string &filename);
    
    // Parse matrix multiplication expression
    std::unique_ptr<MatrixExprAST> parseMatrixMultiplication();
    
    // Generate LLVM IR from the AST
    bool generateIR(std::unique_ptr<ExprAST> ast, llvm::Module *module, llvm::IRBuilder<> &builder);

private:
    // Lexer state
    std::string SourceCode;
    size_t CurPos;
    char CurChar;
    std::string CurToken;
    
    // LLVM context
    llvm::LLVMContext &Context;
    
    // Lexer functions
    void getNextChar();
    void getNextToken();
    bool expectToken(const std::string &token);
    
    // Parser functions
    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parsePrimary();
    std::unique_ptr<ExprAST> parseIdentifier();
    std::unique_ptr<ExprAST> parseMatrixDeclaration();
    std::unique_ptr<ExprAST> parseMatrixOperation();
    
    // Helper functions
    bool isAlpha(char c);
    bool isDigit(char c);
    bool isAlphaNumeric(char c);
    bool isWhitespace(char c);
};

} // namespace ppim

#endif // PPIM_PARSER_H
