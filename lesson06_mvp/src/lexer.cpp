#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

std::string TokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::SELECT:      return "SELECT";
        case TokenType::FROM:        return "FROM";
        case TokenType::WHERE:       return "WHERE";
        case TokenType::INSERT:      return "INSERT";
        case TokenType::INTO:        return "INTO";
        case TokenType::VALUES:      return "VALUES";
        case TokenType::CREATE:      return "CREATE";
        case TokenType::TABLE:       return "TABLE";
        case TokenType::DROP:        return "DROP";
        case TokenType::INT:         return "INT";
        case TokenType::TEXT:        return "TEXT";
        case TokenType::AND:         return "AND";
        case TokenType::OR:          return "OR";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::STR_LITERAL: return "STR_LITERAL";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::STAR:        return "STAR";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::EQ:          return "EQ";
        case TokenType::NEQ:         return "NEQ";
        case TokenType::LT:          return "LT";
        case TokenType::GT:          return "GT";
        case TokenType::LEQ:         return "LEQ";
        case TokenType::GEQ:         return "GEQ";
        case TokenType::EOF_TOKEN:   return "EOF";
        default:                     return "UNKNOWN";
    }
}

std::string Token::ToString() const {
    return TokenTypeToString(type) + "(\"" + value + "\")";
}

Lexer::Lexer(const std::string& sql) : input_(sql), pos_(0), line_(1) {}

char Lexer::Peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }
char Lexer::PeekNext() const { return pos_ + 1 < input_.size() ? input_[pos_ + 1] : '\0'; }
char Lexer::Advance() { char c = input_[pos_++]; if (c == '\n') line_++; return c; }

void Lexer::SkipWhitespace() {
    while (pos_ < input_.size() && std::isspace(Peek())) Advance();
}

TokenType Lexer::CheckKeyword(const std::string& word) {
    std::string upper = word;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "SELECT") return TokenType::SELECT;
    if (upper == "FROM")   return TokenType::FROM;
    if (upper == "WHERE")  return TokenType::WHERE;
    if (upper == "INSERT") return TokenType::INSERT;
    if (upper == "INTO")   return TokenType::INTO;
    if (upper == "VALUES") return TokenType::VALUES;
    if (upper == "CREATE") return TokenType::CREATE;
    if (upper == "TABLE")  return TokenType::TABLE;
    if (upper == "DROP")   return TokenType::DROP;
    if (upper == "INT")    return TokenType::INT;
    if (upper == "TEXT")   return TokenType::TEXT;
    if (upper == "AND")    return TokenType::AND;
    if (upper == "OR")     return TokenType::OR;
    return TokenType::IDENTIFIER;
}

Token Lexer::ReadIdentifierOrKeyword() {
    int sl = line_; std::string word;
    while (pos_ < input_.size() && (std::isalnum(Peek()) || Peek() == '_')) word += Advance();
    return Token(CheckKeyword(word), word, sl);
}

Token Lexer::ReadIntLiteral() {
    int sl = line_; std::string num;
    while (pos_ < input_.size() && std::isdigit(Peek())) num += Advance();
    return Token(TokenType::INT_LITERAL, num, sl);
}

Token Lexer::ReadStrLiteral() {
    int sl = line_; Advance();
    std::string str;
    while (pos_ < input_.size() && Peek() != '\'') {
        if (Peek() == '\\' && PeekNext() == '\'') Advance();
        str += Advance();
    }
    if (pos_ >= input_.size()) throw std::runtime_error("字符串字面量未闭合");
    Advance();
    return Token(TokenType::STR_LITERAL, str, sl);
}

Token Lexer::ReadSymbol() {
    int sl = line_; char c = Advance();
    switch (c) {
        case ',': return Token(TokenType::COMMA,     ",", sl);
        case '(': return Token(TokenType::LPAREN,    "(", sl);
        case ')': return Token(TokenType::RPAREN,    ")", sl);
        case '*': return Token(TokenType::STAR,      "*", sl);
        case ';': return Token(TokenType::SEMICOLON, ";", sl);
        case '=': return Token(TokenType::EQ,        "=", sl);
        case '<': if (Peek()=='='){Advance();return Token(TokenType::LEQ,"<=",sl);}
                  return Token(TokenType::LT,"<",sl);
        case '>': if (Peek()=='='){Advance();return Token(TokenType::GEQ,">=",sl);}
                  return Token(TokenType::GT,">",sl);
        case '!': if (Peek()=='='){Advance();return Token(TokenType::NEQ,"!=",sl);}
                  break;
        default: break;
    }
    return Token(TokenType::UNKNOWN, std::string(1, c), sl);
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (true) {
        SkipWhitespace();
        if (pos_ >= input_.size()) { tokens.emplace_back(TokenType::EOF_TOKEN, "", line_); break; }
        char c = Peek();
        if (std::isalpha(c) || c == '_') tokens.push_back(ReadIdentifierOrKeyword());
        else if (std::isdigit(c))        tokens.push_back(ReadIntLiteral());
        else if (c == '\'')              tokens.push_back(ReadStrLiteral());
        else                             tokens.push_back(ReadSymbol());
    }
    return tokens;
}
