#include "frontend/parser/lexer.h"
#include <unordered_map>

namespace ppim {

// Keywords map
static std::unordered_map<std::string, TokenType> Keywords = {
    {"matrix", tok_matrix},
    {"multiply", tok_multiply}
};

Lexer::Lexer(const std::string &source) 
    : SourceCode(source), CurPos(0), CurChar(' ') {
    if (!source.empty()) {
        CurChar = source[0];
    }
}

void Lexer::getNextChar() {
    CurPos++;
    if (CurPos >= SourceCode.size()) {
        CurChar = 0;  // EOF
    } else {
        CurChar = SourceCode[CurPos];
    }
}

bool Lexer::isAtEnd() const {
    return CurPos >= SourceCode.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || CurChar != expected) {
        return false;
    }
    getNextChar();
    return true;
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

void Lexer::skipWhitespace() {
    while (true) {
        if (CurChar == ' ' || CurChar == '\t' || CurChar == '\r') {
            getNextChar();
        } else if (CurChar == '\n') {
            getNextChar();
        } else if (CurChar == '/') {
            if (CurPos + 1 < SourceCode.size() && SourceCode[CurPos + 1] == '/') {
                // Line comment
                while (CurChar != '\n' && !isAtEnd()) {
                    getNextChar();
                }
            } else if (CurPos + 1 < SourceCode.size() && SourceCode[CurPos + 1] == '*') {
                // Block comment
                getNextChar(); // consume '/'
                getNextChar(); // consume '*'
                while (!isAtEnd() && !(CurChar == '*' && CurPos + 1 < SourceCode.size() && SourceCode[CurPos + 1] == '/')) {
                    getNextChar();
                }
                if (!isAtEnd()) {
                    getNextChar(); // consume '*'
                    getNextChar(); // consume '/'
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

Token Lexer::identifier() {
    std::string lexeme;
    lexeme += CurChar;
    getNextChar();
    
    while (isAlphaNumeric(CurChar)) {
        lexeme += CurChar;
        getNextChar();
    }
    
    // Check if the identifier is a keyword
    if (Keywords.find(lexeme) != Keywords.end()) {
        return Token(Keywords[lexeme], lexeme);
    }
    
    return Token(tok_identifier, lexeme);
}

Token Lexer::number() {
    std::string lexeme;
    int value = 0;
    
    while (isDigit(CurChar)) {
        lexeme += CurChar;
        value = value * 10 + (CurChar - '0');
        getNextChar();
    }
    
    return Token(tok_number, lexeme, value);
}

Token Lexer::getNextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return Token(tok_eof, "EOF");
    }
    
    if (isAlpha(CurChar)) {
        return identifier();
    }
    
    if (isDigit(CurChar)) {
        return number();
    }
    
    // Handle operators and punctuation
    switch (CurChar) {
        case '*':
            getNextChar();
            return Token(tok_multiply_op, "*");
        case ';':
            getNextChar();
            return Token(tok_semicolon, ";");
        case ',':
            getNextChar();
            return Token(tok_comma, ",");
        case '(':
            getNextChar();
            return Token(tok_left_paren, "(");
        case ')':
            getNextChar();
            return Token(tok_right_paren, ")");
        case '[':
            getNextChar();
            return Token(tok_left_bracket, "[");
        case ']':
            getNextChar();
            return Token(tok_right_bracket, "]");
        default: {
            // Unknown token
            std::string lexeme(1, CurChar);
            getNextChar();
            return Token(tok_eof, lexeme); // Return EOF for unknown tokens
        }
    }
}

Token Lexer::peekToken() const {
    // Save current state
    size_t savedPos = CurPos;
    char savedChar = CurChar;
    
    // Create a non-const copy of this lexer
    Lexer copy(*this);
    
    // Get the next token
    Token token = copy.getNextToken();
    
    // Restore state
    const_cast<Lexer*>(this)->CurPos = savedPos;
    const_cast<Lexer*>(this)->CurChar = savedChar;
    
    return token;
}

} // namespace ppim
