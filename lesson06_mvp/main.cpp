#include <iostream>
#include <string>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "executor.h"

// ============================================================
// main.cpp — Lesson 06 完整 MVP 数据库
//
// 整合前5课所有组件，实现：
//   - 交互式 SQL REPL
//   - 数据持久化到磁盘（data/ 目录）
//   - 支持 CREATE TABLE, INSERT, SELECT, DROP TABLE
// ============================================================

// 数据目录
static const std::string DATA_DIR = "lesson06_data";

// 打印欢迎信息
void PrintWelcome() {
    std::cout << "============================================\n";
    std::cout << "  SQL MVP 数据库 — 从零实现数据库引擎\n";
    std::cout << "============================================\n";
    std::cout << "数据目录: " << DATA_DIR << "/\n";
    std::cout << "\n支持的命令:\n";
    std::cout << "  CREATE TABLE name (col TYPE, ...)\n";
    std::cout << "  INSERT INTO name VALUES (v1, v2, ...)\n";
    std::cout << "  SELECT */cols FROM name [WHERE col op val]\n";
    std::cout << "  DROP TABLE name\n";
    std::cout << "  .tables        -- 列出所有表\n";
    std::cout << "  .demo          -- 运行演示\n";
    std::cout << "  .exit          -- 退出\n";
    std::cout << "============================================\n\n";
}

// 运行内置演示
void RunDemo(Catalog& /*catalog*/, ExecutionEngine& engine) {
    std::cout << "\n====== 内置演示开始 ======\n\n";

    std::vector<std::string> demo_sqls = {
        "CREATE TABLE employees (id INT, name TEXT, dept TEXT, salary INT)",
        "INSERT INTO employees VALUES (1, 'Alice', 'Engineering', 90000)",
        "INSERT INTO employees VALUES (2, 'Bob', 'Marketing', 75000)",
        "INSERT INTO employees VALUES (3, 'Charlie', 'Engineering', 85000)",
        "INSERT INTO employees VALUES (4, 'Dave', 'HR', 70000)",
        "INSERT INTO employees VALUES (5, 'Eve', 'Engineering', 95000)",
        "SELECT * FROM employees",
        "SELECT name, salary FROM employees WHERE salary > 80000",
        "SELECT * FROM employees WHERE dept = 'Engineering'",
        "DROP TABLE employees",
    };

    for (const auto& sql : demo_sqls) {
        std::cout << "sql> " << sql << "\n";
        try {
            Lexer lexer(sql);
            auto tokens = lexer.Tokenize();
            Parser parser(tokens);
            auto stmt = parser.Parse();
            std::string result = engine.Execute(stmt);
            if (!result.empty() && result[0] != '(') {
                std::cout << result << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "错误: " << e.what() << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "====== 内置演示结束 ======\n\n";
}

int main(int argc, char* argv[]) {
    PrintWelcome();

    // 初始化 Catalog（自动从磁盘加载已有数据）
    Catalog catalog(DATA_DIR);
    ExecutionEngine engine(&catalog);

    // 显示已有的表
    auto existing_tables = catalog.ListTables();
    if (!existing_tables.empty()) {
        std::cout << "已恢复 " << existing_tables.size() << " 个表: ";
        for (size_t i = 0; i < existing_tables.size(); i++) {
            std::cout << existing_tables[i];
            if (i + 1 < existing_tables.size()) std::cout << ", ";
        }
        std::cout << "\n\n";
    }

    // 如果有命令行参数 --demo，直接运行演示
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        RunDemo(catalog, engine);
        return 0;
    }

    // 交互式 REPL
    std::string line;
    std::string sql_buffer;  // 支持多行 SQL

    while (true) {
        // 打印提示符
        if (sql_buffer.empty()) {
            std::cout << "sql> ";
        } else {
            std::cout << "   > ";  // 多行输入提示
        }
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            // EOF（Ctrl+D）
            std::cout << "\n再见！\n";
            break;
        }

        // 去掉首尾空白
        size_t start = line.find_first_not_of(" \t");
        size_t end   = line.find_last_not_of(" \t");
        if (start == std::string::npos) {
            // 空行
            if (!sql_buffer.empty()) sql_buffer += " ";
            continue;
        }
        line = line.substr(start, end - start + 1);

        // 处理元命令（以 . 开头）
        if (line[0] == '.') {
            if (line == ".exit" || line == ".quit") {
                std::cout << "再见！\n";
                break;
            } else if (line == ".tables") {
                auto tables = catalog.ListTables();
                if (tables.empty()) {
                    std::cout << "（没有表）\n";
                } else {
                    for (const auto& t : tables) std::cout << t << "\n";
                }
            } else if (line == ".demo") {
                RunDemo(catalog, engine);
            } else if (line == ".help") {
                PrintWelcome();
            } else {
                std::cout << "未知命令: " << line << "（输入 .help 查看帮助）\n";
            }
            sql_buffer.clear();
            continue;
        }

        // 累积 SQL（直到遇到分号）
        sql_buffer += (sql_buffer.empty() ? "" : " ") + line;

        // 检查是否有完整的语句（以分号结尾）
        size_t semi_pos = sql_buffer.find(';');
        while (semi_pos != std::string::npos) {
            std::string sql = sql_buffer.substr(0, semi_pos + 1);
            sql_buffer = sql_buffer.substr(semi_pos + 1);
            // 去掉 sql_buffer 开头的空白
            size_t ns = sql_buffer.find_first_not_of(" \t");
            if (ns != std::string::npos) sql_buffer = sql_buffer.substr(ns);
            else sql_buffer.clear();

            // 去掉 sql 开头和结尾的空白
            size_t ss = sql.find_first_not_of(" \t");
            if (ss == std::string::npos) { semi_pos = sql_buffer.find(';'); continue; }
            sql = sql.substr(ss);

            try {
                Lexer lexer(sql);
                auto tokens = lexer.Tokenize();
                Parser parser(tokens);
                auto stmt = parser.Parse();
                std::string result = engine.Execute(stmt);
                std::cout << result << "\n";
            } catch (const std::exception& e) {
                std::cout << "错误: " << e.what() << "\n";
            }

            semi_pos = sql_buffer.find(';');
        }
    }

    // 退出前保存数据
    catalog.SaveAll();
    return 0;
}
