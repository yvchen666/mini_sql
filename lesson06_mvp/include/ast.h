#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

// 列定义
struct ColumnDef {
    std::string name;
    std::string type;
};

// 值类型
using Value = std::variant<int64_t, std::string>;

// 表达式
enum class ExprType { INT_LITERAL, STR_LITERAL, COLUMN_REF, BINARY_OP };

struct Expr {
    ExprType type;
    int64_t int_val = 0;
    std::string str_val;
    std::string op;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;

    static std::shared_ptr<Expr> MakeInt(int64_t v) {
        auto e = std::make_shared<Expr>(); e->type = ExprType::INT_LITERAL; e->int_val = v; return e;
    }
    static std::shared_ptr<Expr> MakeStr(const std::string& s) {
        auto e = std::make_shared<Expr>(); e->type = ExprType::STR_LITERAL; e->str_val = s; return e;
    }
    static std::shared_ptr<Expr> MakeCol(const std::string& col) {
        auto e = std::make_shared<Expr>(); e->type = ExprType::COLUMN_REF; e->str_val = col; return e;
    }
    static std::shared_ptr<Expr> MakeBinary(const std::string& op,
        std::shared_ptr<Expr> l, std::shared_ptr<Expr> r) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::BINARY_OP; e->op = op; e->left = std::move(l); e->right = std::move(r);
        return e;
    }
    std::string ToString() const;
};

// 语句
enum class StmtType { SELECT, INSERT, CREATE_TABLE, DROP_TABLE };

struct Stmt {
    StmtType type;
    // SELECT
    std::vector<std::string> select_columns;
    std::string select_table;
    std::shared_ptr<Expr> where_clause;
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
