#pragma once
#include <string>
#include <vector>

// Token 类型
enum class TokenType {
    SELECT, FROM, WHERE, INSERT, INTO, VALUES,
    CREATE, TABLE, DROP, INT, TEXT, AND, OR,
    IDENTIFIER, INT_LITERAL, STR_LITERAL,
    COMMA, LPAREN, RPAREN, STAR, SEMICOLON,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    EOF_TOKEN, UNKNOWN
};

std::string TokenTypeToString(TokenType type);

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    Token(TokenType t, std::string v, int ln)
        : type(t), value(std::move(v)), line(ln) {}
    std::string ToString() const;
};

class Lexer {
public:
    explicit Lexer(const std::string& sql);
    std::vector<Token> Tokenize();
private:
    std::string input_;
    size_t pos_;
    int    line_;
    char Peek() const;
    char PeekNext() const;
    char Advance();
    void SkipWhitespace();
    Token ReadIdentifierOrKeyword();
    Token ReadIntLiteral();
    Token ReadStrLiteral();
    Token ReadSymbol();
    TokenType CheckKeyword(const std::string& word);
};
