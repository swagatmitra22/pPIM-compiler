#ifndef PPIM_LEXER_H
#define PPIM_LEXER_H

#include <string>
#include <vector>

namespace ppim {

// Token types
enum TokenType {
    tok_eof = -1,
    
    // commands
    tok_matrix = -2,
    tok_multiply = -3,
    
    // primary
    tok_identifier = -4,
    tok_number = -5,
    
    // operators
    tok_multiply_op = -6,
    
    // delimiters
    tok_semicolon = -7,
    tok_comma = -8,
    tok_left_paren = -9,
    tok_right_paren = -10,
    tok_left_bracket = -11,
    tok_right_bracket = -12
};

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    int value;  // For numeric tokens
    
    Token(TokenType t, const std::string &l) : type(t), lexeme(l), value(0) {}
    Token(TokenType t, const std::string &l, int v) : type(t), lexeme(l), value(v) {}
};

// Lexer class
class Lexer {
public:
    Lexer(const std::string &source);
    
    // Get the next token from the source
    Token getNextToken();
    
    // Peek at the current token without consuming it
    Token peekToken() const;
    
private:
    std::string SourceCode;
    size_t CurPos;
    char CurChar;
    
    // Helper methods
    void getNextChar();
    bool isAtEnd() const;
    bool match(char expected);
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Token processing methods
    Token identifier();
    Token number();
    
    // Skip whitespace and comments
    void skipWhitespace();
};

} // namespace ppim

#endif // PPIM_LEXER_H
