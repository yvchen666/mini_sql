#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

// ============================================================
// ast.h — 抽象语法树（AST）节点定义
//
// AST 是 SQL 语句的树形表示，是解析器的输出、执行器的输入。
// 每种 SQL 语句对应一种 Stmt 节点，
// 每种表达式对应一种 Expr 节点。
// ============================================================

// ---- 列定义（用于 CREATE TABLE）----
struct ColumnDef {
    std::string name;   // 列名
    std::string type;   // 类型：INT 或 TEXT
};

// ---- 值（用于 INSERT VALUES）----
// 值可以是整数或字符串
using Value = std::variant<int64_t, std::string>;

// ---- 表达式节点 ----

// 表达式类型
enum class ExprType {
    INT_LITERAL,   // 整数字面量
    STR_LITERAL,   // 字符串字面量
    COLUMN_REF,    // 列引用（列名）
    BINARY_OP,     // 二元运算（=, !=, <, >, <=, >=）
};

struct Expr {
    ExprType type;

    // INT_LITERAL
    int64_t int_val = 0;

    // STR_LITERAL / COLUMN_REF
    std::string str_val;

    // BINARY_OP
    std::string op;                    // 运算符：=, !=, <, >, <=, >=
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;

    // 工厂函数
    static std::shared_ptr<Expr> MakeInt(int64_t v) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::INT_LITERAL;
        e->int_val = v;
        return e;
    }

    static std::shared_ptr<Expr> MakeStr(const std::string& s) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::STR_LITERAL;
        e->str_val = s;
        return e;
    }

    static std::shared_ptr<Expr> MakeCol(const std::string& col) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::COLUMN_REF;
        e->str_val = col;
        return e;
    }

    static std::shared_ptr<Expr> MakeBinary(
        const std::string& op,
        std::shared_ptr<Expr> l,
        std::shared_ptr<Expr> r)
    {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::BINARY_OP;
        e->op = op;
        e->left = std::move(l);
        e->right = std::move(r);
        return e;
    }

    std::string ToString() const;
};

// ---- 语句节点 ----

enum class StmtType {
    SELECT,
    INSERT,
    CREATE_TABLE,
    DROP_TABLE,
};

struct Stmt {
    StmtType type;

    // SELECT
    std::vector<std::string> select_columns;  // 列名列表，"*" 表示全部
    std::string select_table;
    std::shared_ptr<Expr> where_clause;       // 可为 nullptr（无 WHERE）

    // INSERT
    std::string insert_table;
    std::vector<Value> insert_values;

    // CREATE TABLE
    std::string create_table_name;
    std::vector<ColumnDef> create_columns;

    // DROP TABLE
    std::string drop_table_name;

    std::string ToString() const;
};
