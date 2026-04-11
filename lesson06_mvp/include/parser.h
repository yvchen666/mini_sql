#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::shared_ptr<Stmt> Parse();
private:
    std::vector<Token> tokens_;
    size_t pos_;
    const Token& Peek() const;
    Token Advance();
    Token Expect(TokenType expected);
    bool Check(TokenType type) const;
    bool Match(TokenType type);
    std::shared_ptr<Stmt> ParseSelect();
    std::shared_ptr<Stmt> ParseInsert();
    std::shared_ptr<Stmt> ParseCreate();
    std::shared_ptr<Stmt> ParseDrop();
    std::shared_ptr<Expr> ParseExpression();
    std::shared_ptr<Expr> ParseComparison();
    std::shared_ptr<Expr> ParsePrimary();
    std::vector<ColumnDef> ParseColumnDefs();
    std::vector<Value> ParseValueList();
    std::vector<std::string> ParseColumnList();
};
