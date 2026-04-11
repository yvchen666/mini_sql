#pragma once

#include "lexer.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

// ============================================================
// parser.h — 语法分析器（Parser）
//
// 语法分析器接收 Token 序列，按照 SQL 语法规则，
// 构建出 AST（抽象语法树）。
//
// 本实现使用"递归下降解析"（Recursive Descent Parsing）：
// 每种语法规则对应一个解析函数，函数之间互相调用。
//
// 支持的语法：
//   SELECT [* | col1, col2, ...] FROM table [WHERE expr]
//   INSERT INTO table VALUES (val1, val2, ...)
//   CREATE TABLE name (col1 TYPE, col2 TYPE, ...)
//   DROP TABLE name
// ============================================================

class Parser {
public:
    // 输入 Token 序列（由 Lexer 产生）
    explicit Parser(std::vector<Token> tokens);

    // 解析一条 SQL 语句，返回 AST 节点
    // 如果解析失败，抛出 std::runtime_error
    std::shared_ptr<Stmt> Parse();

private:
    std::vector<Token> tokens_;  // Token 序列
    size_t pos_;                 // 当前读取位置

    // ---- Token 操作 ----

    // 查看当前 Token（不消费）
    const Token& Peek() const;

    // 查看下一个 Token（lookahead）
    const Token& PeekNext() const;

    // 消费当前 Token 并返回
    Token Advance();

    // 检查当前 Token 类型是否为 expected，是则消费并返回，否则抛出异常
    Token Expect(TokenType expected);

    // 检查当前 Token 类型是否为 expected（不消费）
    bool Check(TokenType type) const;

    // 如果当前 Token 类型匹配，消费并返回 true；否则返回 false
    bool Match(TokenType type);

    // ---- 语句解析 ----
    std::shared_ptr<Stmt> ParseSelect();
    std::shared_ptr<Stmt> ParseInsert();
    std::shared_ptr<Stmt> ParseCreate();
    std::shared_ptr<Stmt> ParseDrop();

    // ---- 表达式解析 ----
    std::shared_ptr<Expr> ParseExpression();
    std::shared_ptr<Expr> ParseComparison();
    std::shared_ptr<Expr> ParsePrimary();

    // ---- 辅助 ----
    // 解析列定义列表：(col1 TYPE, col2 TYPE, ...)
    std::vector<ColumnDef> ParseColumnDefs();

    // 解析值列表：(val1, val2, ...)
    std::vector<Value> ParseValueList();

    // 解析列名列表：col1, col2, ... 或 *
    std::vector<std::string> ParseColumnList();
};
