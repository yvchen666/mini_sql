#include "parser.h"
#include <sstream>

// ============================================================
// ast.cpp 内容内联在此（ToString 实现）
// ============================================================

std::string Expr::ToString() const {
    switch (type) {
        case ExprType::INT_LITERAL:
            return std::to_string(int_val);
        case ExprType::STR_LITERAL:
            return "'" + str_val + "'";
        case ExprType::COLUMN_REF:
            return str_val;
        case ExprType::BINARY_OP:
            return "(" + left->ToString() + " " + op + " " + right->ToString() + ")";
    }
    return "?";
}

std::string Stmt::ToString() const {
    std::ostringstream oss;
    switch (type) {
        case StmtType::SELECT:
            oss << "SELECT ";
            for (size_t i = 0; i < select_columns.size(); i++) {
                oss << select_columns[i];
                if (i + 1 < select_columns.size()) oss << ", ";
            }
            oss << " FROM " << select_table;
            if (where_clause) oss << " WHERE " << where_clause->ToString();
            break;
        case StmtType::INSERT:
            oss << "INSERT INTO " << insert_table << " VALUES (";
            for (size_t i = 0; i < insert_values.size(); i++) {
                if (std::holds_alternative<int64_t>(insert_values[i]))
                    oss << std::get<int64_t>(insert_values[i]);
                else
                    oss << "'" << std::get<std::string>(insert_values[i]) << "'";
                if (i + 1 < insert_values.size()) oss << ", ";
            }
            oss << ")";
            break;
        case StmtType::CREATE_TABLE:
            oss << "CREATE TABLE " << create_table_name << " (";
            for (size_t i = 0; i < create_columns.size(); i++) {
                oss << create_columns[i].name << " " << create_columns[i].type;
                if (i + 1 < create_columns.size()) oss << ", ";
            }
            oss << ")";
            break;
        case StmtType::DROP_TABLE:
            oss << "DROP TABLE " << drop_table_name;
            break;
    }
    return oss.str();
}

// ============================================================
// parser.cpp — 语法分析器实现
// ============================================================

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), pos_(0) {}

const Token& Parser::Peek() const {
    return tokens_[pos_];
}

const Token& Parser::PeekNext() const {
    if (pos_ + 1 < tokens_.size()) return tokens_[pos_ + 1];
    return tokens_.back();
}

Token Parser::Advance() {
    Token t = tokens_[pos_];
    if (pos_ + 1 < tokens_.size()) pos_++;
    return t;
}

Token Parser::Expect(TokenType expected) {
    if (Peek().type != expected) {
        throw std::runtime_error(
            "语法错误（行 " + std::to_string(Peek().line) + "）：期望 " +
            TokenTypeToString(expected) + "，实际得到 " +
            TokenTypeToString(Peek().type) + "(\"" + Peek().value + "\")");
    }
    return Advance();
}

bool Parser::Check(TokenType type) const {
    return Peek().type == type;
}

bool Parser::Match(TokenType type) {
    if (Check(type)) { Advance(); return true; }
    return false;
}

std::shared_ptr<Stmt> Parser::Parse() {
    std::shared_ptr<Stmt> stmt;
    switch (Peek().type) {
        case TokenType::SELECT: stmt = ParseSelect(); break;
        case TokenType::INSERT: stmt = ParseInsert(); break;
        case TokenType::CREATE: stmt = ParseCreate(); break;
        case TokenType::DROP:   stmt = ParseDrop();   break;
        default:
            throw std::runtime_error(
                "语法错误：不支持的语句类型 \"" + Peek().value + "\"");
    }
    // 可选的分号
    Match(TokenType::SEMICOLON);
    return stmt;
}

// SELECT [* | col1, col2, ...] FROM table [WHERE expr]
std::shared_ptr<Stmt> Parser::ParseSelect() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::SELECT;

    Expect(TokenType::SELECT);
    stmt->select_columns = ParseColumnList();
    Expect(TokenType::FROM);
    stmt->select_table = Expect(TokenType::IDENTIFIER).value;

    if (Match(TokenType::WHERE)) {
        stmt->where_clause = ParseExpression();
    }

    return stmt;
}

// INSERT INTO table VALUES (val1, val2, ...)
std::shared_ptr<Stmt> Parser::ParseInsert() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::INSERT;

    Expect(TokenType::INSERT);
    Expect(TokenType::INTO);
    stmt->insert_table = Expect(TokenType::IDENTIFIER).value;
    Expect(TokenType::VALUES);
    stmt->insert_values = ParseValueList();

    return stmt;
}

// CREATE TABLE name (col1 TYPE, col2 TYPE, ...)
std::shared_ptr<Stmt> Parser::ParseCreate() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::CREATE_TABLE;

    Expect(TokenType::CREATE);
    Expect(TokenType::TABLE);
    stmt->create_table_name = Expect(TokenType::IDENTIFIER).value;
    stmt->create_columns = ParseColumnDefs();

    return stmt;
}

// DROP TABLE name
std::shared_ptr<Stmt> Parser::ParseDrop() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::DROP_TABLE;

    Expect(TokenType::DROP);
    Expect(TokenType::TABLE);
    stmt->drop_table_name = Expect(TokenType::IDENTIFIER).value;

    return stmt;
}

// 解析列名列表：* 或 col1, col2, ...
std::vector<std::string> Parser::ParseColumnList() {
    std::vector<std::string> cols;
    if (Check(TokenType::STAR)) {
        Advance();
        cols.push_back("*");
        return cols;
    }
    cols.push_back(Expect(TokenType::IDENTIFIER).value);
    while (Match(TokenType::COMMA)) {
        cols.push_back(Expect(TokenType::IDENTIFIER).value);
    }
    return cols;
}

// 解析列定义：(col1 TYPE, col2 TYPE, ...)
std::vector<ColumnDef> Parser::ParseColumnDefs() {
    std::vector<ColumnDef> defs;
    Expect(TokenType::LPAREN);
    do {
        ColumnDef def;
        def.name = Expect(TokenType::IDENTIFIER).value;
        // 类型：INT 或 TEXT
        if (Check(TokenType::INT)) {
            def.type = "INT";
            Advance();
        } else if (Check(TokenType::TEXT)) {
            def.type = "TEXT";
            Advance();
        } else {
            throw std::runtime_error("语法错误：期望列类型 INT 或 TEXT");
        }
        defs.push_back(def);
    } while (Match(TokenType::COMMA));
    Expect(TokenType::RPAREN);
    return defs;
}

// 解析值列表：(val1, val2, ...)
std::vector<Value> Parser::ParseValueList() {
    std::vector<Value> vals;
    Expect(TokenType::LPAREN);
    do {
        if (Check(TokenType::INT_LITERAL)) {
            vals.push_back(static_cast<int64_t>(std::stoll(Advance().value)));
        } else if (Check(TokenType::STR_LITERAL)) {
            vals.push_back(Advance().value);
        } else {
            throw std::runtime_error("语法错误：期望值（整数或字符串）");
        }
    } while (Match(TokenType::COMMA));
    Expect(TokenType::RPAREN);
    return vals;
}

// 解析表达式（支持 AND/OR 逻辑运算）
std::shared_ptr<Expr> Parser::ParseExpression() {
    auto left = ParseComparison();
    while (Check(TokenType::AND) || Check(TokenType::OR)) {
        std::string op = Advance().value;
        auto right = ParseComparison();
        left = Expr::MakeBinary(op, left, right);
    }
    return left;
}

// 解析比较表达式：primary op primary
std::shared_ptr<Expr> Parser::ParseComparison() {
    auto left = ParsePrimary();
    if (Check(TokenType::EQ) || Check(TokenType::NEQ) ||
        Check(TokenType::LT) || Check(TokenType::GT) ||
        Check(TokenType::LEQ) || Check(TokenType::GEQ)) {
        std::string op = Advance().value;
        auto right = ParsePrimary();
        return Expr::MakeBinary(op, left, right);
    }
    return left;
}

// 解析基本表达式：字面量或列引用
std::shared_ptr<Expr> Parser::ParsePrimary() {
    if (Check(TokenType::INT_LITERAL)) {
        int64_t v = std::stoll(Advance().value);
        return Expr::MakeInt(v);
    }
    if (Check(TokenType::STR_LITERAL)) {
        std::string s = Advance().value;
        return Expr::MakeStr(s);
    }
    if (Check(TokenType::IDENTIFIER)) {
        std::string col = Advance().value;
        return Expr::MakeCol(col);
    }
    if (Match(TokenType::LPAREN)) {
        auto e = ParseExpression();
        Expect(TokenType::RPAREN);
        return e;
    }
    throw std::runtime_error("语法错误：期望表达式，得到 \"" + Peek().value + "\"");
}
