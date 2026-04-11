#include "parser.h"
#include <sstream>
#include <stdexcept>

// ---- AST ToString ----
std::string Expr::ToString() const {
    switch (type) {
        case ExprType::INT_LITERAL: return std::to_string(int_val);
        case ExprType::STR_LITERAL: return "'" + str_val + "'";
        case ExprType::COLUMN_REF:  return str_val;
        case ExprType::BINARY_OP:   return "(" + left->ToString() + " " + op + " " + right->ToString() + ")";
    }
    return "?";
}

std::string Stmt::ToString() const {
    std::ostringstream oss;
    switch (type) {
        case StmtType::SELECT:
            oss << "SELECT ";
            for (size_t i = 0; i < select_columns.size(); i++) {
                oss << select_columns[i]; if (i+1<select_columns.size()) oss << ", ";
            }
            oss << " FROM " << select_table;
            if (where_clause) oss << " WHERE " << where_clause->ToString();
            break;
        case StmtType::INSERT:
            oss << "INSERT INTO " << insert_table << " VALUES (";
            for (size_t i = 0; i < insert_values.size(); i++) {
                if (std::holds_alternative<int64_t>(insert_values[i]))
                    oss << std::get<int64_t>(insert_values[i]);
                else oss << "'" << std::get<std::string>(insert_values[i]) << "'";
                if (i+1<insert_values.size()) oss << ", ";
            }
            oss << ")";
            break;
        case StmtType::CREATE_TABLE:
            oss << "CREATE TABLE " << create_table_name << " (";
            for (size_t i = 0; i < create_columns.size(); i++) {
                oss << create_columns[i].name << " " << create_columns[i].type;
                if (i+1<create_columns.size()) oss << ", ";
            }
            oss << ")";
            break;
        case StmtType::DROP_TABLE:
            oss << "DROP TABLE " << drop_table_name;
            break;
    }
    return oss.str();
}

// ---- Parser ----
Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), pos_(0) {}

const Token& Parser::Peek() const { return tokens_[pos_]; }

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
            Peek().value + "\"");
    }
    return Advance();
}

bool Parser::Check(TokenType type) const { return Peek().type == type; }

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
            throw std::runtime_error("不支持的语句: \"" + Peek().value + "\"");
    }
    Match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<Stmt> Parser::ParseSelect() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::SELECT;
    Expect(TokenType::SELECT);
    stmt->select_columns = ParseColumnList();
    Expect(TokenType::FROM);
    stmt->select_table = Expect(TokenType::IDENTIFIER).value;
    if (Match(TokenType::WHERE)) stmt->where_clause = ParseExpression();
    return stmt;
}

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

std::shared_ptr<Stmt> Parser::ParseCreate() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::CREATE_TABLE;
    Expect(TokenType::CREATE);
    Expect(TokenType::TABLE);
    stmt->create_table_name = Expect(TokenType::IDENTIFIER).value;
    stmt->create_columns = ParseColumnDefs();
    return stmt;
}

std::shared_ptr<Stmt> Parser::ParseDrop() {
    auto stmt = std::make_shared<Stmt>();
    stmt->type = StmtType::DROP_TABLE;
    Expect(TokenType::DROP);
    Expect(TokenType::TABLE);
    stmt->drop_table_name = Expect(TokenType::IDENTIFIER).value;
    return stmt;
}

std::vector<std::string> Parser::ParseColumnList() {
    std::vector<std::string> cols;
    if (Check(TokenType::STAR)) { Advance(); cols.push_back("*"); return cols; }
    cols.push_back(Expect(TokenType::IDENTIFIER).value);
    while (Match(TokenType::COMMA)) cols.push_back(Expect(TokenType::IDENTIFIER).value);
    return cols;
}

std::vector<ColumnDef> Parser::ParseColumnDefs() {
    std::vector<ColumnDef> defs;
    Expect(TokenType::LPAREN);
    do {
        ColumnDef def;
        def.name = Expect(TokenType::IDENTIFIER).value;
        if (Check(TokenType::INT)) { def.type = "INT"; Advance(); }
        else if (Check(TokenType::TEXT)) { def.type = "TEXT"; Advance(); }
        else throw std::runtime_error("期望列类型 INT 或 TEXT");
        defs.push_back(def);
    } while (Match(TokenType::COMMA));
    Expect(TokenType::RPAREN);
    return defs;
}

std::vector<Value> Parser::ParseValueList() {
    std::vector<Value> vals;
    Expect(TokenType::LPAREN);
    do {
        if (Check(TokenType::INT_LITERAL))
            vals.push_back(static_cast<int64_t>(std::stoll(Advance().value)));
        else if (Check(TokenType::STR_LITERAL))
            vals.push_back(Advance().value);
        else throw std::runtime_error("期望值（整数或字符串）");
    } while (Match(TokenType::COMMA));
    Expect(TokenType::RPAREN);
    return vals;
}

std::shared_ptr<Expr> Parser::ParseExpression() {
    auto left = ParseComparison();
    while (Check(TokenType::AND) || Check(TokenType::OR)) {
        std::string op = Advance().value;
        auto right = ParseComparison();
        left = Expr::MakeBinary(op, left, right);
    }
    return left;
}

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

std::shared_ptr<Expr> Parser::ParsePrimary() {
    if (Check(TokenType::INT_LITERAL)) return Expr::MakeInt(std::stoll(Advance().value));
    if (Check(TokenType::STR_LITERAL)) return Expr::MakeStr(Advance().value);
    if (Check(TokenType::IDENTIFIER))  return Expr::MakeCol(Advance().value);
    if (Match(TokenType::LPAREN)) {
        auto e = ParseExpression();
        Expect(TokenType::RPAREN);
        return e;
    }
    throw std::runtime_error("期望表达式，得到 \"" + Peek().value + "\"");
}
