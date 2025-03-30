#include "frontend/parser/parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "llvm/IR/Verifier.h"

namespace ppim {

Parser::Parser(llvm::LLVMContext &context) 
    : Context(context) {}

Parser::~Parser() {}

std::unique_ptr<ExprAST> Parser::parseFile(const std::string &filename) {
    // Read the file content
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();
    
    // Initialize lexer
    lexer = std::make_unique<Lexer>(sourceCode);
    getNextToken(); // Initialize currentToken
    
    // Parse the file content
    std::vector<std::unique_ptr<ExprAST>> expressions;
    
    while (currentToken.type != tok_eof) {
        auto expr = parseExpression();
        if (!expr) {
            return nullptr;
        }
        expressions.push_back(std::move(expr));
        
        // Expect semicolon after each expression
        if (currentToken.type == tok_semicolon) {
            getNextToken(); // consume semicolon
        }
    }
    
    return std::make_unique<BlockExprAST>(std::move(expressions));
}

void Parser::getNextToken() {
    currentToken = lexer->getNextToken();
}

bool Parser::expectToken(TokenType type) {
    if (currentToken.type != type) {
        std::cerr << "Expected token type " << type << ", got " << currentToken.type 
                  << " (" << currentToken.lexeme << ")" << std::endl;
        return false;
    }
    getNextToken();
    return true;
}

std::unique_ptr<ExprAST> Parser::parseExpression() {
    if (currentToken.type == tok_matrix) {
        return parseMatrixDeclaration();
    } else if (currentToken.type == tok_multiply) {
        return parseMatrixOperation();
    } else {
        std::cerr << "Unexpected token: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    if (currentToken.type == tok_number) {
        int val = currentToken.value;
        getNextToken();
        return std::make_unique<NumberExprAST>(val);
    } else if (currentToken.type == tok_identifier) {
        return parseIdentifier();
    } else {
        std::cerr << "Unknown token: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
}

std::unique_ptr<ExprAST> Parser::parseIdentifier() {
    std::string name = currentToken.lexeme;
    getNextToken();
    return std::make_unique<VariableExprAST>(name);
}

std::unique_ptr<ExprAST> Parser::parseMatrixDeclaration() {
    // Parse: matrix <name> <rows> <cols> [elements...]
    getNextToken(); // consume 'matrix'
    
    if (currentToken.type != tok_identifier) {
        std::cerr << "Expected matrix name, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    std::string name = currentToken.lexeme;
    getNextToken();
    
    if (currentToken.type != tok_number) {
        std::cerr << "Expected number of rows, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    int rows = currentToken.value;
    getNextToken();
    
    if (currentToken.type != tok_number) {
        std::cerr << "Expected number of columns, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    int cols = currentToken.value;
    getNextToken();
    
    // Parse matrix elements
    std::vector<int> elements;
    
    // Check if we have a left bracket for matrix elements
    if (currentToken.type == tok_left_bracket) {
        getNextToken(); // consume '['
        
        // Parse elements until we reach the right bracket
        while (currentToken.type != tok_right_bracket) {
            if (currentToken.type != tok_number) {
                std::cerr << "Expected matrix element, got: " << currentToken.lexeme << std::endl;
                return nullptr;
            }
            
            elements.push_back(currentToken.value);
            getNextToken();
            
            // Check for comma between elements
            if (currentToken.type == tok_comma) {
                getNextToken(); // consume ','
            } else if (currentToken.type != tok_right_bracket) {
                std::cerr << "Expected comma or right bracket, got: " << currentToken.lexeme << std::endl;
                return nullptr;
            }
        }
        
        getNextToken(); // consume ']'
    } else {
        // If no elements are provided, initialize with zeros
        elements.resize(rows * cols, 0);
    }
    
    // Validate element count
    if (elements.size() != rows * cols) {
        std::cerr << "Matrix element count mismatch: expected " << (rows * cols) 
                  << ", got " << elements.size() << std::endl;
        return nullptr;
    }
    
    return std::make_unique<MatrixDeclExprAST>(name, rows, cols, std::move(elements));
}

std::unique_ptr<ExprAST> Parser::parseMatrixOperation() {
    // Parse: multiply <matrix1> <matrix2> <result>
    getNextToken(); // consume 'multiply'
    
    if (currentToken.type != tok_identifier) {
        std::cerr << "Expected matrix name, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    std::string lhsName = currentToken.lexeme;
    getNextToken();
    
    if (currentToken.type != tok_identifier) {
        std::cerr << "Expected matrix name, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    std::string rhsName = currentToken.lexeme;
    getNextToken();
    
    if (currentToken.type != tok_identifier) {
        std::cerr << "Expected result matrix name, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    std::string resultName = currentToken.lexeme;
    getNextToken();
    
    // Create matrix expressions for the operands
    // Note: In a real implementation, we would look up the dimensions from a symbol table
    // For now, we'll set them to 0 and resolve them later
    auto lhs = std::make_unique<MatrixExprAST>(lhsName, 0, 0);
    auto rhs = std::make_unique<MatrixExprAST>(rhsName, 0, 0);
    
    return std::make_unique<MatrixMultExprAST>(std::move(lhs), std::move(rhs), resultName);
}

std::unique_ptr<MatrixExprAST> Parser::parseMatrixMultiplication() {
    // This is a simplified version for the specific case of matrix multiplication
    if (currentToken.type != tok_multiply) {
        std::cerr << "Expected 'multiply' keyword, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    getNextToken(); // consume 'multiply'
    
    if (currentToken.type != tok_identifier) {
        std::cerr << "Expected matrix name, got: " << currentToken.lexeme << std::endl;
        return nullptr;
    }
    
    std::string name = currentToken.lexeme;
    getNextToken();
    
    // In a real implementation, we would look up the dimensions from a symbol table
    return std::make_unique<MatrixExprAST>(name, 0, 0);
}

bool Parser::generateIR(std::unique_ptr<ExprAST> ast, llvm::Module *module, llvm::IRBuilder<> &builder) {
    if (!ast) {
        return false;
    }
    
    // Generate code from the AST
    llvm::Value *value = ast->codegen();
    if (!value) {
        std::cerr << "Code generation failed" << std::endl;
        return false;
    }
    
    // Verify the generated code
    bool broken = llvm::verifyModule(*module, &llvm::errs());
    if (broken) {
        std::cerr << "Module verification failed" << std::endl;
        return false;
    }
    
    return true;
}

// Helper function to parse input file
std::unique_ptr<ExprAST> parseInputFile(const std::string &filename) {
    llvm::LLVMContext context;
    Parser parser(context);
    return parser.parseFile(filename);
}

// Helper function to generate IR
bool generateIR(std::unique_ptr<ExprAST> ast, llvm::Module *module, llvm::IRBuilder<> &builder) {
    if (!ast) {
        return false;
    }
    
    llvm::LLVMContext &context = module->getContext();
    Parser parser(context);
    return parser.generateIR(std::move(ast), module, builder);
}

} // namespace ppim
