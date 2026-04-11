#include <iostream>
#include "catalog.h"
#include "executor.h"

// ============================================================
// main.cpp — Lesson 05 查询执行器演示
// ============================================================

int main() {
    std::cout << "=== Lesson 05: 查询执行器（火山模型）===\n\n";

    // 初始化 Catalog 和执行引擎
    Catalog catalog;
    ExecutionEngine engine(&catalog);

    // ---- 演示 1：CREATE TABLE ----
    std::cout << "--- CREATE TABLE users ---\n";
    Schema users_schema;
    users_schema.table_name = "users";
    users_schema.columns = {
        {"id",   "INT"},
        {"name", "TEXT"},
        {"age",  "INT"}
    };
    engine.ExecuteCreate(users_schema);
    std::cout << "表 users 创建成功\n\n";

    // ---- 演示 2：INSERT ----
    std::cout << "--- INSERT INTO users ---\n";
    std::vector<Tuple> insert_data = {
        {int64_t(1), std::string("Alice"),   int64_t(30)},
        {int64_t(2), std::string("Bob"),     int64_t(25)},
        {int64_t(3), std::string("Charlie"), int64_t(35)},
        {int64_t(4), std::string("Dave"),    int64_t(28)},
        {int64_t(5), std::string("Eve"),     int64_t(22)},
    };

    for (auto& row : insert_data) {
        engine.ExecuteInsert("users", row);
        std::cout << "插入: " << ValueToString(row[0]) << ", "
                  << ValueToString(row[1]) << ", "
                  << ValueToString(row[2]) << "\n";
    }

    // ---- 演示 3：SELECT * ----
    std::cout << "\n--- SELECT * FROM users ---\n";
    {
        SimpleSelect sel;
        sel.table = "users";
        sel.columns = {"*"};
        auto result = engine.ExecuteSelect(sel);

        // 获取 schema 用于打印
        auto* td = catalog.GetTable("users");
        ExecutionEngine::PrintResult(result, td->schema);
    }

    // ---- 演示 4：SELECT with WHERE（大于）----
    std::cout << "\n--- SELECT * FROM users WHERE age > 27 ---\n";
    {
        SimpleSelect sel;
        sel.table = "users";
        sel.columns = {"*"};
        sel.where = SimpleExpr{
            SimpleExpr::Type::COMPARE,
            "age", ">", int64_t(27)
        };
        auto result = engine.ExecuteSelect(sel);
        auto* td = catalog.GetTable("users");
        ExecutionEngine::PrintResult(result, td->schema);
    }

    // ---- 演示 5：SELECT 指定列 with WHERE（等值）----
    std::cout << "\n--- SELECT name, age FROM users WHERE id = 3 ---\n";
    {
        SimpleSelect sel;
        sel.table = "users";
        sel.columns = {"name", "age"};
        sel.where = SimpleExpr{
            SimpleExpr::Type::COMPARE,
            "id", "=", int64_t(3)
        };
        auto result = engine.ExecuteSelect(sel);
        // 投影后的 schema
        Schema proj_schema;
        proj_schema.columns = {{"name", "TEXT"}, {"age", "INT"}};
        ExecutionEngine::PrintResult(result, proj_schema);
    }

    // ---- 演示 6：第二张表 ----
    std::cout << "\n--- 创建并查询 orders 表 ---\n";
    Schema orders_schema;
    orders_schema.table_name = "orders";
    orders_schema.columns = {
        {"order_id", "INT"},
        {"user_id",  "INT"},
        {"amount",   "INT"}
    };
    engine.ExecuteCreate(orders_schema);

    engine.ExecuteInsert("orders", {int64_t(101), int64_t(1), int64_t(500)});
    engine.ExecuteInsert("orders", {int64_t(102), int64_t(2), int64_t(300)});
    engine.ExecuteInsert("orders", {int64_t(103), int64_t(1), int64_t(200)});
    engine.ExecuteInsert("orders", {int64_t(104), int64_t(3), int64_t(800)});

    {
        SimpleSelect sel;
        sel.table = "orders";
        sel.columns = {"*"};
        sel.where = SimpleExpr{
            SimpleExpr::Type::COMPARE,
            "amount", ">", int64_t(250)
        };
        auto result = engine.ExecuteSelect(sel);
        auto* td = catalog.GetTable("orders");
        ExecutionEngine::PrintResult(result, td->schema);
    }

    // ---- 演示 7：DROP TABLE ----
    std::cout << "\n--- DROP TABLE orders ---\n";
    engine.ExecuteDrop("orders");
    std::cout << "表 orders 已删除\n";

    // 验证表不存在
    try {
        SimpleSelect sel;
        sel.table = "orders";
        sel.columns = {"*"};
        engine.ExecuteSelect(sel);
    } catch (const std::exception& e) {
        std::cout << "预期错误：" << e.what() << "\n";
    }

    std::cout << "\n=== Lesson 05 完成！===\n";
    std::cout << "下一步：学习 Lesson 06，将所有组件整合成完整的数据库系统。\n";
    return 0;
}
