#pragma once

#include <string>
#include <vector>

// ============================================================
// lexer.h — 词法分析器（Lexer / Tokenizer）
//
// 词法分析是编译的第一步：
// 将原始字符串分解为有意义的"词法单元"（Token）序列。
//
// 例如：
//   输入：  "SELECT name FROM users"
//   输出：  [SELECT, IDENTIFIER("name"), FROM, IDENTIFIER("users"), EOF]
// ============================================================

// ============================================================
// TokenType — Token 的类型枚举
// ============================================================
enum class TokenType {
    // 关键字
    SELECT, FROM, WHERE, INSERT, INTO, VALUES,
    CREATE, TABLE, DROP,
    INT, TEXT,      // 数据类型关键字
    AND, OR,        // 逻辑运算符关键字

    // 标识符和字面量
    IDENTIFIER,     // 表名、列名等
    INT_LITERAL,    // 整数字面量，如 42
    STR_LITERAL,    // 字符串字面量，如 'hello'

    // 符号
    COMMA,          // ,
    LPAREN,         // (
    RPAREN,         // )
    STAR,           // *
    SEMICOLON,      // ;
    EQ,             // =
    NEQ,            // !=
    LT,             // <
    GT,             // >
    LEQ,            // <=
    GEQ,            // >=

    // 特殊
    EOF_TOKEN,      // 输入结束
    UNKNOWN         // 未知字符（错误）
};

// 将 TokenType 转换为可读字符串（调试用）
std::string TokenTypeToString(TokenType type);

// ============================================================
// Token — 单个词法单元
// ============================================================
struct Token {
    TokenType   type;     // Token 类型
    std::string value;    // Token 的原始文本
    int         line;     // 所在行号（错误报告用）

    Token(TokenType t, std::string v, int ln)
        : type(t), value(std::move(v)), line(ln) {}

    std::string ToString() const;
};

// ============================================================
// Lexer — 词法分析器
// ============================================================
class Lexer {
public:
    // 输入 SQL 字符串，初始化词法分析器
    explicit Lexer(const std::string& sql);

    // 对整个输入进行词法分析，返回 Token 序列
    std::vector<Token> Tokenize();

private:
    std::string input_;   // 原始 SQL 字符串
    size_t pos_;          // 当前读取位置
    int    line_;         // 当前行号

    // 查看当前字符但不消费
    char Peek() const;

    // 查看下一个字符（lookahead）
    char PeekNext() const;

    // 消费当前字符并返回
    char Advance();

    // 跳过空白字符（空格、制表符、换行）
    void SkipWhitespace();

    // 读取关键字或标识符
    Token ReadIdentifierOrKeyword();

    // 读取整数字面量
    Token ReadIntLiteral();

    // 读取字符串字面量（单引号包围）
    Token ReadStrLiteral();

    // 读取符号/运算符
    Token ReadSymbol();

    // 判断字符串是否为 SQL 关键字
    TokenType CheckKeyword(const std::string& word);
};
