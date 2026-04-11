#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>

// ============================================================
// lexer.cpp — 词法分析器实现
// ============================================================

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
    return TokenTypeToString(type) + "(\"" + value + "\", line=" + std::to_string(line) + ")";
}

// ---- Lexer 实现 ----

Lexer::Lexer(const std::string& sql) : input_(sql), pos_(0), line_(1) {}

char Lexer::Peek() const {
    if (pos_ >= input_.size()) return '\0';
    return input_[pos_];
}

char Lexer::PeekNext() const {
    if (pos_ + 1 >= input_.size()) return '\0';
    return input_[pos_ + 1];
}

char Lexer::Advance() {
    char c = input_[pos_++];
    if (c == '\n') line_++;
    return c;
}

void Lexer::SkipWhitespace() {
    while (pos_ < input_.size() && std::isspace(Peek())) {
        Advance();
    }
}

TokenType Lexer::CheckKeyword(const std::string& word) {
    // 转大写后比较（SQL 关键字不区分大小写）
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
    int start_line = line_;
    std::string word;
    // 标识符：字母或下划线开头，后跟字母、数字、下划线
    while (pos_ < input_.size() && (std::isalnum(Peek()) || Peek() == '_')) {
        word += Advance();
    }
    TokenType type = CheckKeyword(word);
    return Token(type, word, start_line);
}

Token Lexer::ReadIntLiteral() {
    int start_line = line_;
    std::string num;
    while (pos_ < input_.size() && std::isdigit(Peek())) {
        num += Advance();
    }
    return Token(TokenType::INT_LITERAL, num, start_line);
}

Token Lexer::ReadStrLiteral() {
    int start_line = line_;
    Advance();  // 跳过开头的单引号 '
    std::string str;
    while (pos_ < input_.size() && Peek() != '\'') {
        if (Peek() == '\\' && PeekNext() == '\'') {
            Advance();  // 跳过转义符
        }
        str += Advance();
    }
    if (pos_ >= input_.size()) {
        throw std::runtime_error("词法错误：字符串字面量未闭合（行 " + std::to_string(start_line) + "）");
    }
    Advance();  // 跳过结尾的单引号 '
    return Token(TokenType::STR_LITERAL, str, start_line);
}

Token Lexer::ReadSymbol() {
    int start_line = line_;
    char c = Advance();
    switch (c) {
        case ',': return Token(TokenType::COMMA,     ",", start_line);
        case '(': return Token(TokenType::LPAREN,    "(", start_line);
        case ')': return Token(TokenType::RPAREN,    ")", start_line);
        case '*': return Token(TokenType::STAR,      "*", start_line);
        case ';': return Token(TokenType::SEMICOLON, ";", start_line);
        case '=': return Token(TokenType::EQ,        "=", start_line);
        case '<':
            if (Peek() == '=') { Advance(); return Token(TokenType::LEQ, "<=", start_line); }
            return Token(TokenType::LT, "<", start_line);
        case '>':
            if (Peek() == '=') { Advance(); return Token(TokenType::GEQ, ">=", start_line); }
            return Token(TokenType::GT, ">", start_line);
        case '!':
            if (Peek() == '=') { Advance(); return Token(TokenType::NEQ, "!=", start_line); }
            break;
        default: break;
    }
    return Token(TokenType::UNKNOWN, std::string(1, c), start_line);
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;

    while (true) {
        SkipWhitespace();

        if (pos_ >= input_.size()) {
            tokens.emplace_back(TokenType::EOF_TOKEN, "", line_);
            break;
        }

        char c = Peek();

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(ReadIdentifierOrKeyword());
        } else if (std::isdigit(c)) {
            tokens.push_back(ReadIntLiteral());
        } else if (c == '\'') {
            tokens.push_back(ReadStrLiteral());
        } else {
            tokens.push_back(ReadSymbol());
        }
    }

    return tokens;
}
