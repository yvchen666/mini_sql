#include <iostream>
#include <vector>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

// ============================================================
// main.cpp — Lesson 04 SQL 解析器演示
// ============================================================

// 解析并打印一条 SQL 语句
void ParseAndPrint(const std::string& sql) {
    std::cout << "\n输入 SQL: " << sql << "\n";
    try {
        // 词法分析
        Lexer lexer(sql);
        auto tokens = lexer.Tokenize();

        std::cout << "Token 序列: ";
        for (const auto& tok : tokens) {
            if (tok.type != TokenType::EOF_TOKEN) {
                std::cout << "[" << tok.value << "] ";
            }
        }
        std::cout << "\n";

        // 语法分析
        Parser parser(tokens);
        auto stmt = parser.Parse();

        std::cout << "AST 输出: " << stmt->ToString() << "\n";
    } catch (const std::exception& e) {
        std::cout << "错误: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "=== Lesson 04: SQL 解析器 ===\n";

    // ---- 演示 1：CREATE TABLE ----
    std::cout << "\n--- CREATE TABLE ---\n";
    ParseAndPrint("CREATE TABLE users (id INT, name TEXT, age INT)");

    // ---- 演示 2：INSERT ----
    std::cout << "\n--- INSERT ---\n";
    ParseAndPrint("INSERT INTO users VALUES (1, 'Alice', 30)");
    ParseAndPrint("INSERT INTO users VALUES (2, 'Bob', 25)");

    // ---- 演示 3：SELECT * ----
    std::cout << "\n--- SELECT * ---\n";
    ParseAndPrint("SELECT * FROM users");

    // ---- 演示 4：SELECT 带 WHERE ----
    std::cout << "\n--- SELECT with WHERE ---\n";
    ParseAndPrint("SELECT name, age FROM users WHERE id = 1");
    ParseAndPrint("SELECT * FROM users WHERE age > 20");

    // ---- 演示 5：DROP TABLE ----
    std::cout << "\n--- DROP TABLE ---\n";
    ParseAndPrint("DROP TABLE users");

    // ---- 演示 6：大小写不敏感 ----
    std::cout << "\n--- 大小写不敏感 ---\n";
    ParseAndPrint("select * from users where id = 42");

    // ---- 演示 7：错误处理 ----
    std::cout << "\n--- 错误处理 ---\n";
    ParseAndPrint("SELECT FROM users");  // 缺少列名
    ParseAndPrint("INSERT users VALUES (1)");  // 缺少 INTO

    std::cout << "\n=== Lesson 04 完成！===\n";
    std::cout << "下一步：学习 Lesson 05，了解如何执行解析后的 SQL 语句。\n";
    return 0;
}
